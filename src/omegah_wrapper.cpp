#include <Omega_h_array_ops.hpp>
#include <Omega_h_comm.hpp>
#include <Omega_h_file.hpp>
#include <Omega_h_for.hpp>
#include <Omega_h_library.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_scalar.hpp>

#include "redev_wrapper.h"
#include <redev.h>

#include "test_support.h"

struct omegah_mesh;

namespace ts = test_support;

extern "C" struct omegah_mesh *new_oh_mesh(const char *meshFile)
{
    int argc = 0;
    char **argv = NULL;
    static auto lib = Omega_h::Library(&argc, &argv);
    static auto world = lib.world();
    static Omega_h::Mesh mesh(&lib);
    Omega_h::binary::read(meshFile, lib.world(), &mesh);
    return ((struct omegah_mesh *)&mesh);
}

extern "C" struct rdv_ptn *create_oh_partition(struct omegah_mesh *meshp,
                                               const char *cpnFile)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;

    auto ohComm = mesh->comm();
    ts::ClassificationPartition ptn;

    if(cpnFile) {
        const auto facePartition = !ohComm->rank()
                                       ? ts::readClassPartitionFile(cpnFile)
                                       : ts::ClassificationPartition();
        ts::migrateMeshElms(*mesh, facePartition);
        ptn = ts::CreateClassificationPartition(*mesh);
        // rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,true);
    } else {
        ptn = ts::ClassificationPartition();
        // rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,false);
    }

    return ((struct rdv_ptn *)(new redev::ClassPtn(MPI_COMM_WORLD, ptn.ranks,
                                                   ptn.modelEnts)));
}

extern "C" void get_omegah_layout(struct omegah_mesh *meshp,
                                  struct rdv_ptn *rptn, uint32_t **dest,
                                  uint32_t **offset, size_t *num_dest)
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

static OMEGA_H_DEVICE Omega_h::I8 isModelEntInOverlap(const int dim,
                                                      const int id)
{
    // the TOMMS generated geometric model has
    // entity IDs that increase with the distance
    // from the magnetic axis
    if(dim == 2 && (id >= 22 && id <= 34)) {
        return 1;
    } else if(dim == 1 && (id >= 21 && id <= 34)) {
        return 1;
    } else if(dim == 0 && (id >= 21 && id <= 34)) {
        return 1;
    }
    return 0;
}

extern "C" void mark_mesh_overlap(struct omegah_mesh *meshp, int min_class,
                                  int max_class)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;

    auto classIds = mesh->get_array<Omega_h::ClassId>(0, "class_id");
    auto classDims = mesh->get_array<Omega_h::I8>(0, "class_dim");
    auto isOverlap = Omega_h::Write<Omega_h::I8>(classIds.size(), "isOverlap");

    auto markOverlap = OMEGA_H_LAMBDA(int i)
    {
        isOverlap[i] = isModelEntInOverlap(classDims[i], classIds[i]);
    };
    Omega_h::parallel_for(classIds.size(), markOverlap);
    auto isOverlap_r = Omega_h::read(isOverlap);
    mesh->add_tag(0, "isOverlap", 1, isOverlap_r);
}

extern "C" int get_mesh_nverts(struct omegah_mesh *meshp)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;
    return(mesh->nverts());
}
