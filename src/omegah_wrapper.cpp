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

//#include <wdmcpl.h>

struct omegah_mesh {
    omegah_mesh(const char *meshFile, int *argc_, char ***argv_) : lib(argc_, argv_), mesh(&lib) {
        Omega_h::binary::read(meshFile, lib.world(), &mesh);    
    }

    Omega_h::Library lib;
    Omega_h::Mesh mesh;
};
struct omegah_array;

namespace ts = test_support;

extern "C" struct omegah_mesh *new_oh_mesh(const char *meshFile)
{
    int argc = 0;
    char **argv = NULL;
    struct omegah_mesh *mesh = new struct omegah_mesh(meshFile, &argc, &argv);
    std::cout << "struct addr: " << (void *)mesh << " mesh address: " << (void *)&mesh->mesh << std::endl;
    return(mesh);
}

extern "C" struct rdv_ptn *create_oh_partition(struct omegah_mesh *mesh,
                                               const char *cpnFile)
{
    auto ohComm = mesh->mesh.comm();
    ts::ClassificationPartition ptn;

    if(cpnFile) {
        const auto facePartition = !ohComm->rank()
                                       ? ts::readClassPartitionFile(cpnFile)
                                       : ts::ClassificationPartition();
        ts::migrateMeshElms(mesh->mesh, facePartition);
        ptn = ts::CreateClassificationPartition(mesh->mesh);
        // rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,true);
    } else {
        ptn = ts::ClassificationPartition();
        // rdv = redev::Redev(MPI_COMM_WORLD,rdv_ptn,false);
    }

    return ((struct rdv_ptn *)(new redev::ClassPtn(MPI_COMM_WORLD, ptn.ranks,
                                                   ptn.modelEnts)));
}

extern "C" void get_omegah_layout(struct omegah_mesh *mesh,
                                  struct rdv_ptn *rptn, uint32_t **dest,
                                  uint32_t **offset, size_t *num_dest)
{
    redev::ClassPtn *ptn = (redev::ClassPtn *)rptn;

    ts::OutMsg appOut = ts::prepareAppOutMessage(mesh->mesh, *ptn);
    *num_dest = appOut.dest.size();
    *dest = (uint32_t *)malloc(sizeof(**dest) * *num_dest);
    *offset = (uint32_t *)malloc(sizeof(**dest) * (*num_dest + 1));
    memcpy(*dest, appOut.dest.data(), sizeof(**dest) * *num_dest);
    memcpy(*offset, appOut.offset.data(), sizeof(**dest) * (*num_dest + 1));
}

static OMEGA_H_DEVICE Omega_h::I8 isModelEntInOverlap(const int dim,
                                                      const int id, 
                                                      const int min_class,
                                                      const int max_class)
{
    // the TOMMS generated geometric model has
    // entity IDs that increase with the distance
    // from the magnetic axis
    if(dim == 2 && (id >= min_class && id <= max_class)) {
        return 1;
    } else if(dim == 1 && (id >= min_class && id <= min_class)) {
        return 1;
    } else if(dim == 0 && (id >= min_class && id <= max_class)) {
        return 1;
    }
    return 0;
}

extern "C" struct omegah_array *mark_mesh_overlap(struct omegah_mesh *mesh, int min_class,
                                  int max_class)
{
    auto classIds = mesh->mesh.get_array<Omega_h::ClassId>(0, "class_id");
    auto classDims = mesh->mesh.get_array<Omega_h::I8>(0, "class_dim");
    auto isOverlap = Omega_h::Write<Omega_h::I8>(classIds.size(), "isOverlap");

    auto markOverlap = OMEGA_H_LAMBDA(int i)
    {
        isOverlap[i] = 1;
        //isOverlap[i] = isModelEntInOverlap(classDims[i], classIds[i], min_class, max_class);
    };
    Omega_h::parallel_for(classIds.size(), markOverlap);
    auto isOwned = mesh->mesh.owned(0);
    
    // try masking out to only owned entities
    Omega_h::parallel_for(
        isOverlap.size(),
        OMEGA_H_LAMBDA(int i) { isOverlap[i] = (isOwned[i] && isOverlap[i]); });
    
    Omega_h::Read<Omega_h::I8> *isOverlap_r = new Omega_h::Read<Omega_h::I8>(Omega_h::read(isOverlap));
    mesh->mesh.add_tag(0, "isOverlap", 1, *isOverlap_r);
    return((struct omegah_array *)isOverlap_r);
}

extern "C" struct omegah_array *mark_server_mesh_overlap(struct omegah_mesh *mesh, struct rdv_ptn *rptn, int min_class, int max_class)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    redev::ClassPtn *ptn = (redev::ClassPtn *)rptn;

    // transfer vtx classification to host
    std::cout << "in mark, mesh address is " << (void *)&mesh->mesh << std::endl;
    auto classIds = mesh->mesh.get_array<Omega_h::ClassId>(0, "class_id");
    auto classIds_h = Omega_h::HostRead(classIds);
    auto classDims = mesh->mesh.get_array<Omega_h::I8>(0, "class_dim");
    auto classDims_h = Omega_h::HostRead(classDims);
    auto isOverlap = Omega_h::Write<Omega_h::I8>(classIds.size(), "isOverlap");
    auto markOverlap = OMEGA_H_LAMBDA(int i)
    {
        isOverlap[i] = 1;
        //isOverlap[i] = isModelEntInOverlap(classDims[i], classIds[i], min_class, max_class);
    };
    Omega_h::parallel_for(classIds.size(), markOverlap);
    auto owned_h = Omega_h::HostRead(mesh->mesh.owned(0));
    auto isOverlap_h = Omega_h::HostRead<Omega_h::I8>(isOverlap);

    // mask to only class partition owned entities
    auto isOverlapOwned = Omega_h::HostWrite<Omega_h::I8>(classIds.size(), "isOverlapAndOwnsModelEntInClassPartition");
    for(int i=0; i < mesh->mesh.nverts(); i++) {
        redev::ClassPtn::ModelEnt ent(classDims_h[i],classIds_h[i]);
        auto destRank = ptn->GetRank(ent);
        auto isModelEntOwned = (destRank == rank);
        isOverlapOwned[i] = isModelEntOwned && isOverlap_h[i];
        if( owned_h[i] && !isModelEntOwned ) {
            fprintf(stderr, "%d owner conflict %d ent (%d,%d) owner %d owned %d\n",
            rank, i, classDims_h[i], classIds_h[i], destRank, owned_h[i]);
        }
    }

    Omega_h::Read<Omega_h::I8> *isOverlapOwned_dr = new Omega_h::Read<Omega_h::I8>(Omega_h::read(Omega_h::Write(isOverlapOwned)));
    mesh->mesh.add_tag(0, "isOverlap", 1, *isOverlapOwned_dr);
    return((struct omegah_array *)isOverlapOwned_dr);
}

extern "C" int get_mesh_nverts(struct omegah_mesh *mesh)
{
    return(mesh->mesh.nverts());
}

extern "C" void *get_mesh(struct omegah_mesh *mesh)
{
    return((void *)&mesh->mesh);
}

extern "C" void close_mesh(struct omegah_mesh *mesh)
{
    delete(mesh);
}
