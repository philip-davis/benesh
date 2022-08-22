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

struct omegah_mesh;
struct omegah_array;

/*
struct OmegaHGids
{
  OmegaHGids(Omega_h::Mesh& mesh, Omega_h::HostRead<Omega_h::I8> is_overlap_h)
    : mesh_(mesh), is_overlap_h_(is_overlap_h)
  {
  }
  std::vector<redev::GO> operator()(std::string_view) const
  {
    auto gids = mesh_.globals(0);
    auto gids_h = Omega_h::HostRead(gids);
    std::vector<redev::GO> global_ids;
    for (size_t i = 0; i < gids_h.size(); i++) {
      if (is_overlap_h_[i]) {
        global_ids.push_back(gids_h[i]);
      }
    }
    return global_ids;
  }
  Omega_h::Mesh mesh_;
  Omega_h::HostRead<Omega_h::I8> is_overlap_h_;
};

struct SerializeOmegaHGids
{
  SerializeOmegaHGids(Omega_h::Mesh& mesh,
                      Omega_h::HostRead<Omega_h::I8> is_overlap_h)
    : mesh_(mesh), is_overlap_h_(is_overlap_h)
  {
  }
  template <typename T>
  int operator()(std::string_view, nonstd::span<T> buffer,
                 nonstd::span<const wdmcpl::LO> permutation) const
  {
    // WDMCPL_ALWAYS_ASSERT(buffer.size() == is_overlap_h_.size());
    auto gids = mesh_.globals(0);
    auto gids_h = Omega_h::HostRead(gids);
    int count = 0;
    for (size_t i = 0, j = 0; i < gids_h.size(); i++) {
      if (is_overlap_h_[i]) {
        if (buffer.size() > 0) {
          buffer[permutation[j++]] = gids_h[i];
        }
        ++count;
      }
    }
    return count;
  }
  Omega_h::Mesh mesh_;
  Omega_h::HostRead<Omega_h::I8> is_overlap_h_;
};

// Serializer is used in a two pass algorithm. Must check that the buffer size
// >0 and return the number of entries.
struct SerializeOmegaH
{
  SerializeOmegaH(Omega_h::Mesh& mesh,
                  Omega_h::HostRead<Omega_h::I8> is_overlap_h)
    : mesh_(mesh), is_overlap_h_(is_overlap_h)
  {
  }
  template <typename T>
  int operator()(std::string_view name, nonstd::span<T> buffer,
                 nonstd::span<const wdmcpl::LO> permutation) const
  {
    // WDMCPL_ALWAYS_ASSERT(buffer.size() == is_overlap_h_.size());
    const auto array = mesh_.get_array<T>(0, std::string(name));
    const auto array_h = Omega_h::HostRead(array);
    int count = 0;
    for (size_t i = 0, j = 0; i < array_h.size(); i++) {
      if (is_overlap_h_[i]) {
        if (buffer.size() > 0) {
          buffer[permutation[j++]] = array_h[i];
        }
        ++count;
      }
    }
    return count;
  }
  Omega_h::Mesh mesh_;
  Omega_h::HostRead<Omega_h::I8> is_overlap_h_;
};

struct DeserializeOmegaH
{
  DeserializeOmegaH(Omega_h::Mesh& mesh,
                    Omega_h::HostRead<Omega_h::I8> is_overlap_h)
    : mesh_(mesh), is_overlap_h_(is_overlap_h)
  {
  }
  template <typename T>
  void operator()(std::string_view, nonstd::span<const T> buffer,
                  nonstd::span<const wdmcpl::LO> permutation) const
  {

    REDEV_ALWAYS_ASSERT(buffer.size() == permutation.size());
    auto gids = mesh_.globals(0);
    auto gids_h = Omega_h::HostRead(gids);
    std::vector<redev::GO> global_ids;
    for (size_t i = 0, j = 0; i < gids_h.size(); i++) {
      if (is_overlap_h_[i]) {
        REDEV_ALWAYS_ASSERT(gids_h[i] == buffer[permutation[j++]]);
      }
    }
  }

private:
  Omega_h::Mesh& mesh_;
  Omega_h::HostRead<Omega_h::I8> is_overlap_h_;
};
*/
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

extern "C" struct omegah_array *mark_mesh_overlap(struct omegah_mesh *meshp, int min_class,
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
    Omega_h::Read<Omega_h::I8> *isOverlap_r = new Omega_h::Read<Omega_h::I8>(Omega_h::read(isOverlap));
    mesh->add_tag(0, "isOverlap", 1, *isOverlap_r);
    return((struct omegah_array *)isOverlap_r);
}

extern "C" struct omegah_array *mark_server_mesh_overlap(struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;
    redev::ClassPtn *ptn = (redev::ClassPtn *)rptn;

    // transfer vtx classification to host
    auto classIds = mesh->get_array<Omega_h::ClassId>(0, "class_id");
    auto classIds_h = Omega_h::HostRead(classIds);
    auto classDims = mesh->get_array<Omega_h::I8>(0, "class_dim");
    auto classDims_h = Omega_h::HostRead(classDims);
    auto isOverlap = Omega_h::Write<Omega_h::I8>(classIds.size(), "isOverlap");
    auto markOverlap = OMEGA_H_LAMBDA(int i)
    {
        isOverlap[i] = isModelEntInOverlap(classDims[i], classIds[i]);
    };
    Omega_h::parallel_for(classIds.size(), markOverlap);
    auto owned_h = Omega_h::HostRead(mesh->owned(0));
    auto isOverlap_h = Omega_h::HostRead<Omega_h::I8>(isOverlap);

    // mask to only class partition owned entities
    auto isOverlapOwned = Omega_h::HostWrite<Omega_h::I8>(classIds.size(), "isOverlapAndOwnsModelEntInClassPartition");
    for(int i=0; i<mesh->nverts(); i++) {
        redev::ClassPtn::ModelEnt ent(classDims_h[i],classIds_h[i]);
        auto destRank = ptn->GetRank(ent);
        auto isModelEntOwned = (destRank == rank);
        isOverlapOwned[i] = isModelEntOwned && isOverlap_h[i];
        if( owned_h[i] && !isModelEntOwned ) {
            fprintf(stderr, "%d owner conflict %d ent (%d,%d) owner %d owned %d\n",
            rank, i, classDims_h[i], classIds_h[i], destRank, owned_h[i]);
        }
    }

    //this is a crime: host -> device -> host
    auto isOverlapOwned_dr = Omega_h::read(Omega_h::Write(isOverlapOwned));
    Omega_h::HostRead<Omega_h::I8> *isOverlapOwned_hr = new Omega_h::HostRead<Omega_h::I8>(Omega_h::HostRead(isOverlapOwned_dr));
    mesh->add_tag(0, "isOverlap", 1, isOverlapOwned_dr);
    return((struct omegah_array *)isOverlapOwned_hr);
}

extern "C" int get_mesh_nverts(struct omegah_mesh *meshp)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)meshp;
    return(mesh->nverts());
}

