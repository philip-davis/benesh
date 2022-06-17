#include <Omega_h_file.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_array_ops.hpp>
#include <Omega_h_comm.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_for.hpp>
#include <Omega_h_scalar.hpp> 

#include <redev.h>
#include "redev_wrapper.h"

#include "test_support.h"

struct omegah_mesh;

namespace ts = test_support;

extern "C" struct omegah_mesh *new_oh_mesh(const char *meshFile, const char *cpnFile)
{
    static auto lib = Omega_h::Library(NULL, NULL);
    static auto world = lib.world();
    static Omega_h::Mesh mesh(&lib);
    Omega_h::binary::read(meshFile, lib.world(), &mesh);
    return((struct omegah_mesh *)&mesh);
}

extern "C" struct rdv_ptn *create_oh_partition(struct omegah_mesh *meshp, const char *cpnFile)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;

    auto ohComm = mesh->comm();
    ts::ClassificationPartition ptn;

    if(cpnFile) {
        const auto facePartition = !ohComm->rank() ? ts::readClassPartitionFile(cpnFile) :
                                    ts::ClassificationPartition();
        ts::migrateMeshElms(*mesh, facePartition);
        ptn = ts::CreateClassificationPartition(*mesh);
        //rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,true);
    } else {
        ptn = ts::ClassificationPartition();
        //rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,false);
    }

    return((struct rdv_ptn *)(new redev::ClassPtn(MPI_COMM_WORLD, ptn.ranks, ptn.modelEnts)));
}

extern "C" void get_omegah_layout(struct omegah_mesh *meshp, struct rdv_ptn *rptn, uint32_t **dest, uint32_t **offset, size_t *num_dest)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;
    redev::ClassPtn *ptn = (redev::ClassPtn *)rptn;

    ts::OutMsg appOut = ts::prepareAppOutMessage(*mesh, *ptn);
    *num_dest = appOut.dest.size();
    *dest = (uint32_t *)malloc(sizeof(**dest) * *num_dest);
    *offset = (uint32_t *)malloc(sizeof(**dest) * (*num_dest + 1));
    memcpy(*dest, appOut.dest.data(), sizeof(**dest) * *num_dest);
    memcpy(*offset, appOut.offset.data(), sizeof(**dest) * (*num_dest + 1)); 
}

/*
extern "C" void mark_mesh_overlap(struct omegah_mesh *meshp, int min_class, int max_class)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;
    
}
*/
