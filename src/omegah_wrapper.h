#ifndef _BNH_OH_WRAPPER_
#define _BNH_OH_WRAPPER_

#ifdef USE_APEX
#include <apex.h>
#define APEX_FUNC_TIMER_START(fn)                                              \
    apex_profiler_handle profiler0 = apex_start(APEX_FUNCTION_ADDRESS, &fn);
#define APEX_NAME_TIMER_START(num, name)                                       \
    apex_profiler_handle profiler##num = apex_start(APEX_NAME_STRING, name);
#define APEX_TIMER_STOP(num) apex_stop(profiler##num);
#else
#define APEX_FUNC_TIMER_START(fn) (void)0;
#define APEX_NAME_TIMER_START(num, name) (void)0;
#define APEX_TIMER_STOP(num) (void)0;
#endif

#ifdef __cplusplus
#include<redev.h>
#include<redev_variant_tools.h>

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
                    Omega_h::HostRead<Omega_h::I8> is_overlap_h,
                    void **out_buf,
                    size_t *num_elem)
    : mesh_(mesh), is_overlap_h_(is_overlap_h), out_buf_(out_buf), num_elem_(num_elem)
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
    /*
    if(*num_elem_ != buffer.size()) {
        if(*num_elem_) {
            free(*out_buf_);
        }
        *num_elem_ = buffer.size();
        *out_buf_ = malloc(*num_elem_ * sizeof(T));
    }
    */
    for (size_t i = 0, j = 0; i < gids_h.size(); i++) {
      if (is_overlap_h_[i]) {
        REDEV_ALWAYS_ASSERT(gids_h[i] == buffer[permutation[j++]]);
      }
    }
  }

private:
  Omega_h::Mesh& mesh_;
  Omega_h::HostRead<Omega_h::I8> is_overlap_h_;
  void **out_buf_;
  size_t *num_elem_;
};

struct DeserializeServer
{
  DeserializeServer(std::vector<wdmcpl::GO>& v) : v_(v){};
  template <typename T>
  void operator()(std::string_view name, nonstd::span<const T> buffer,
                  nonstd::span<const wdmcpl::LO> permutation) const
  {
    v_.resize(buffer.size());
    for (int i = 0; i < buffer.size(); ++i) {
      v_[i] = buffer[permutation[i]];
    }
  }

private:
  std::vector<wdmcpl::GO>& v_;
};
struct SerializeServer
{
  SerializeServer(std::vector<wdmcpl::GO>& v) : v_(v){};

  template <typename T>
  int operator()(std::string_view name, nonstd::span<T> buffer,
                 nonstd::span<const wdmcpl::LO> permutation) const
  {
    APEX_NAME_TIMER_START(1, "serialize_server");
    if (buffer.size() >= 0) {
      for (int i = 0; i < buffer.size(); ++i) {
        buffer[permutation[i]] = v_[i];
      }
    }
    APEX_TIMER_STOP(1);
    return v_.size();
  }

private:
  std::vector<wdmcpl::GO>& v_;
};


struct OmegaHReversePartition
{
  OmegaHReversePartition(Omega_h::Mesh& mesh) : mesh(mesh) {}
  wdmcpl::ReversePartitionMap operator()(std::string_view,
                                         const redev::Partition& partition) const
  {
    auto ohComm = mesh.comm();
    const auto rank = ohComm->rank();
    // transfer vtx classification to host
    auto classIds = mesh.get_array<Omega_h::ClassId>(0, "class_id");
    auto classIds_h = Omega_h::HostRead(classIds);
    auto classDims = mesh.get_array<Omega_h::I8>(0, "class_dim");
    auto classDims_h = Omega_h::HostRead(classDims);
    auto isOverlap =
      mesh.has_tag(0, "isOverlap")
        ? mesh.get_array<Omega_h::I8>(0, "isOverlap")
        : Omega_h::Read<Omega_h::I8>(
            classIds.size(), 1, "isOverlap"); // no mask for overlap vertices
    auto isOverlap_h = Omega_h::HostRead(isOverlap);
    // local_index number of vertices going to each destination process by
    // calling getRank - degree array
    wdmcpl::ReversePartitionMap reverse_partition;
    wdmcpl::LO local_index = 0;
    for (auto i = 0; i < classIds_h.size(); i++) {
      if (isOverlap_h[i]) {
        auto dr = std::visit(
          redev::overloaded{
            [&classDims_h, &classIds_h, &i](const redev::ClassPtn& ptn) {
              const auto ent =
                redev::ClassPtn::ModelEnt({classDims_h[i], classIds_h[i]});
              return ptn.GetRank(ent);
            },
            [](const redev::RCBPtn& ptn) {
              std::cerr << "RCB partition not handled yet\n";
              std::exit(EXIT_FAILURE);
              return 0;
            }},
          partition);
        reverse_partition[dr].emplace_back(local_index++);
      }
    }
    return reverse_partition;
  }
  Omega_h::Mesh& mesh;
};


#endif /* __cpluscplus */

struct omegah_mesh *new_oh_mesh(const char *meshFile);

struct rdv_ptn *create_oh_partition(struct omegah_mesh *meshp,
                                    const char *cpnFile);

void get_omegah_layout(struct omegah_mesh *meshp, struct rdv_ptn *rptn,
                       uint32_t **dest, uint32_t **offset, size_t *num_dest);

//FIX ME
#ifndef __cplusplus
struct omegah_array *mark_mesh_overlap(struct omegah_mesh *meshp, int min_class, int max_class);
struct omegah_array *mark_server_mesh_overlap(struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class);
void *get_mesh(struct omegah_mesh *mesh);
#endif

int get_mesh_nverts(struct omegah_mesh *meshp);
void close_mesh(struct omegah_mesh *mesh);

#endif
