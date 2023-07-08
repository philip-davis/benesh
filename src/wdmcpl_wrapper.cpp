#ifndef __BNH_CPL_WRAPPER_H_
#define __BNH_CPL_WRAPPER_H_

#include<mpi.h>

#include<inttypes.h>
#include<vector>
#include<chrono>

#include<wdmcpl/wdmcpl.h>
#include<wdmcpl/dummy_field_adapter.h>
#include<wdmcpl/xgc_field_adapter.h>

#include <Omega_h_mesh.hpp>

#include "omegah_wrapper.h"
#include "redev_wrapper.h"
#include "wrapper_common.h"

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

struct MeanCombiner
{
  void operator()(
    const nonstd::span<
      const std::reference_wrapper<wdmcpl::InternalField>>& fields,
    wdmcpl::InternalField& combined_variant) const
  {
    std::visit(
      [&fields](auto&& combined_field) {
        using T = typename std::remove_reference_t<
          decltype(combined_field)>::value_type;
        Omega_h::Write<T> combined_array(combined_field.Size());
        for (auto& field_variant : fields) {
          std::visit(
            [&combined_array, &combined_field](auto&& field) {
              WDMCPL_ALWAYS_ASSERT(field.Size() == combined_array.size());
              auto field_array = get_nodal_data(field);
              Omega_h::parallel_for(
                field_array.size(),
                OMEGA_H_LAMBDA(int i) { combined_array[i] += field_array[i]; });
            },
            field_variant.get());
        }
        auto num_fields = fields.size();
        Omega_h::parallel_for(
          combined_array.size(),
          OMEGA_H_LAMBDA(int i) { combined_array[i] /= num_fields; });
        set_nodal_data(combined_field,
                       make_array_view(Omega_h::Read(combined_array)));
      },
      combined_variant);
  }
};

static void timeMinMaxAvg(double time, double& min, double& max, double& avg) {
  const auto comm = MPI_COMM_WORLD;
  int nproc;
  MPI_Comm_size(comm, &nproc);
  double tot = 0;
  MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, comm);
  MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&time, &tot, 1, MPI_DOUBLE, MPI_SUM, comm);
  avg = tot / nproc;
}

static void printTime(std::string_view mode, double min, double max, double avg) {
  std::stringstream ss;
  ss << mode << " elapsed time min, max, avg (s): "
   << min << " " << max << " " << avg << "\n";
  std::cout << ss.str();
}

template <class T> void comparePrintTime(T start, T end, std::string_view key, int rank)
{
    std::chrono::duration<double> elapsed_seconds = end - start;
    double min, max, avg;
    timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank)
        printTime(key, min, max, avg);
}


extern "C" struct omegah_array *mark_mesh_overlap(struct omegah_mesh *meshp, int min_class, int max_class);
extern "C" struct omegah_array *mark_server_mesh_overlap(struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class);
extern "C" void *get_mesh(struct omegah_mesh *mesh);

using AdapterVariant = std::variant<std::monostate, wdmcpl::XGCFieldAdapter<double>,
                wdmcpl::XGCFieldAdapter<float>, wdmcpl::XGCFieldAdapter<int>,
                wdmcpl::XGCFieldAdapter<long>, wdmcpl::DummyFieldAdapter>;
using ServerAdapterVariant = std::variant<std::monostate, wdmcpl::OmegaHFieldAdapter<double>, 
                wdmcpl::OmegaHFieldAdapter<long>, wdmcpl::OmegaHFieldAdapter<int>>;

struct field_adapter {
    enum bnh_data_type data_type;
    union {
        AdapterVariant *adpt;
        ServerAdapterVariant *srv_adpt;
    };
};

struct cpl_handl;

struct app_hndl {
    wdmcpl::Application *handle;
    Omega_h::Read<Omega_h::I8> *overlap_h;
    Omega_h::Mesh *mesh;
    std::map<std::string, std::shared_ptr<struct field_adapter>> adpts;
    struct cpl_hndl *cpl_h;
};

struct cpl_hndl {
    union {
        wdmcpl::CouplerServer *cpl_srv;
        wdmcpl::CouplerClient *cpl_client;
    };
    bool server;
    std::unordered_map<std::string, std::reference_wrapper<wdmcpl::ConvertibleCoupledField>> fields_;
    //std::unordered_map<std::string, std::reference_wrapper<wdmcpl::InternalField>> internal_fields_;
    std::unordered_map<std::string, wdmcpl::InternalField> internal_fields_;
    Omega_h::Mesh *mesh;
    int64_t *buffer;
    size_t num_elem;
    std::map<std::string, std::shared_ptr<struct app_hndl>> apps;
};

struct cpl_gid_field {
    /*
    wdmcpl::FieldCommunicatorT<wdmcpl::GO> *comm;
    std::vector<wdmcpl::GO> gid_field;
    */
    wdmcpl::ConvertibleCoupledField *field;
    char *field_name;
    struct cpl_hndl *cpl;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> tsendstart;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> tsendend;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> trecvstart;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> trecvend;
};

extern "C" struct cpl_hndl *create_cpl_hndl(const char *wfname, struct omegah_mesh *meshp, struct rdv_ptn *ptnp, int server, MPI_Comm comm)
{
    auto ptn = (redev::ClassPtn *)ptnp;
    //struct cpl_hndl *cpl_h = (struct cpl_hndl *)malloc(sizeof(*cpl_h));
    struct cpl_hndl *cpl_h = new struct cpl_hndl;
    cpl_h->mesh = (Omega_h::Mesh *)get_mesh(meshp);
    cpl_h->server = (bool)server;
    if(cpl_h->server) {
        cpl_h->cpl_srv = new wdmcpl::CouplerServer(wfname, comm, *ptn, *cpl_h->mesh);
        //cpl_h->fields_ = std::unordered_map<std::string, std::reference_wrapper<wdmcpl::ConvertibleCoupledField>>();
        //cpl_h->internal_fields_ = std::unordered_map<std::string, wdmcpl::InternalField>(); 
    } else {
        cpl_h->cpl_client = new wdmcpl::CouplerClient(wfname, comm);
    }
    return(cpl_h);
}

extern "C" void app_begin_recv_phase(struct app_hndl *app_h)
{
    struct cpl_hndl *cpl_h = app_h->cpl_h;

    if(cpl_h->server) {
        auto *app = app_h->handle;
        app->BeginReceivePhase();
    } else {
        auto *cpl = (wdmcpl::CouplerClient *)cpl_h->cpl_client;
        cpl->BeginReceivePhase();
    }
}

extern "C" void app_end_recv_phase(struct app_hndl *app_h)
{
    struct cpl_hndl *cpl_h = app_h->cpl_h;
    
    if(cpl_h->server) {
        auto *app = app_h->handle; 
        app->EndReceivePhase();
    } else {
        auto *cpl = (wdmcpl::CouplerClient *)cpl_h->cpl_client;
        cpl->EndReceivePhase();
    }
}

extern "C" void cpl_begin_send_phase(struct app_hndl *app_h)
{
    struct cpl_hndl *cpl_h = app_h->cpl_h;
    if(cpl_h->server) {
        auto *app = app_h->handle;
        app->BeginSendPhase();
    } else {
        auto *cpl = (wdmcpl::CouplerClient *)cpl_h->cpl_client;
        cpl->BeginSendPhase();
    }
}

extern "C" void cpl_end_send_phase(struct app_hndl *app_h)
{
    struct cpl_hndl *cpl_h = app_h->cpl_h;
    if(cpl_h->server) {
        auto *app = app_h->handle;
        app->EndSendPhase();
    } else {
        auto *cpl = (wdmcpl::CouplerClient *)cpl_h->cpl_client;
        cpl->EndSendPhase();
    }
}

extern "C" void close_cpl(struct cpl_hndl *cpl_h)
{

    if(cpl_h->server) {
        delete(cpl_h->cpl_srv);
    } else {
        delete(cpl_h->cpl_client);
    }
}

extern "C" struct app_hndl *add_application(struct cpl_hndl *cpl_h, const char *name, const char *path)
{
    struct app_hndl *app = new struct app_hndl;
   
    if(cpl_h->server) {
        auto cpl = (wdmcpl::CouplerServer *)(cpl_h->cpl_srv);
        app->handle = cpl->AddApplication(name, path);
        app->mesh = cpl_h->mesh;
    }
    cpl_h->apps[name] = std::shared_ptr<struct app_hndl>(app);
    app->cpl_h = cpl_h;

    return(app);
}

extern "C" void mark_cpl_overlap(struct cpl_hndl *cpl_h, struct app_hndl *apph, struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class)
{
    if(cpl_h->server) {
        apph->overlap_h = (Omega_h::Read<Omega_h::I8> *)mark_server_mesh_overlap(meshp, rptn, min_class, max_class);
    } else {
        apph->overlap_h = (Omega_h::Read<Omega_h::I8> *)mark_mesh_overlap(meshp, min_class, max_class);
    }
}

extern "C" struct field_adapter *create_omegah_adaptor(struct app_hndl *app_h, const char *name, struct omegah_mesh *meshp, enum bnh_data_type data_type)
{
    struct field_adapter *adpt_h = new struct field_adapter;
    adpt_h->srv_adpt = new ServerAdapterVariant{};
    adpt_h->data_type = data_type;
    switch(data_type) {
        /*
        case BNH_CPL_FLOAT:
            adpt_h->srv_adpt->emplace<wdmcpl::OmegaHFieldAdapter<float>>(name, *app_h->mesh, *app_h->overlap_h, "simNumbering");
           break;
        */
        case BNH_CPL_DOUBLE:
            adpt_h->srv_adpt->emplace<wdmcpl::OmegaHFieldAdapter<double>>(name, *app_h->mesh, *app_h->overlap_h, "simNumbering"); 
            break;
        case BNH_CPL_INT:
            adpt_h->srv_adpt->emplace<wdmcpl::OmegaHFieldAdapter<int>>(name, *app_h->mesh, *app_h->overlap_h, "simNumbering");
            break;
        case BNH_CPL_LONG_INT:
            adpt_h->srv_adpt->emplace<wdmcpl::OmegaHFieldAdapter<long>>(name, *app_h->mesh, *app_h->overlap_h, "simNumbering");
        default:
            std::cerr << "ERROR: bad data type in " << __func__ << std::endl;
            return(NULL);
    }

    app_h->adpts[name] = std::shared_ptr<struct field_adapter>(adpt_h);
    return(adpt_h);
}

static int8_t isModelEntInOverlap(const int dim, const int id,
                                  const int min_class, const int max_class)
{
    if(dim == 2 && (id >= min_class && id <= max_class)) {
        return 1;
    } else if(dim == 1 && (id >= min_class && id <= min_class)) {
        return 1;
    } else if(dim == 0 && (id >= min_class && id <= max_class)) {
        return 1;
    }
    return 0;
}

template <typename T>
void wdmcpl_create_xgc_field_adapter_t(
  const char* name, MPI_Comm comm, void* data, int size,
  const wdmcpl::ReverseClassificationVertex& reverse_classification,
  const int min_class, const int max_class, 
  AdapterVariant& adpt)
{
  WDMCPL_ALWAYS_ASSERT((size >0) ? (data!=nullptr) : true);
  wdmcpl::ScalarArrayView<T, wdmcpl::HostMemorySpace> data_view(
    reinterpret_cast<T*>(data), size);
  adpt.emplace<wdmcpl::XGCFieldAdapter<T>>(
    name, comm, data_view, reverse_classification, 
        [=](int dim, int id) {
             return(isModelEntInOverlap(dim, id, min_class, max_class));
         });
}

struct rcn_handle;

extern "C" struct rcn_handle *get_rcn_from_file(const char *fname, MPI_Comm comm)
{
    return((struct rcn_handle *)(new wdmcpl::ReverseClassificationVertex{wdmcpl::ReadReverseClassificationVertex(fname, comm)}));
}

extern "C" struct field_adapter *create_mpient_adaptor(struct app_hndl *app_h, const char *name, struct rcn_handle *rcn_h, MPI_Comm comm, void *data, int size, enum bnh_data_type data_type, int min_class, int max_class)
{
    struct field_adapter *adpt_h = new struct field_adapter;
    wdmcpl::ReverseClassificationVertex *rcn = (wdmcpl::ReverseClassificationVertex *)rcn_h;
    adpt_h->adpt = new AdapterVariant{};
    adpt_h->data_type = data_type;
    switch(data_type) {
        case BNH_CPL_FLOAT:
            wdmcpl_create_xgc_field_adapter_t<float>(name, comm, data, size, *rcn, min_class, max_class, *adpt_h->adpt);
            break;
        case BNH_CPL_DOUBLE:
            wdmcpl_create_xgc_field_adapter_t<double>(name, comm, data, size, *rcn, min_class, max_class, *adpt_h->adpt);
            break;
        case BNH_CPL_INT:
            wdmcpl_create_xgc_field_adapter_t<int>(name, comm, data, size, *rcn, min_class, max_class, *adpt_h->adpt);
            break;
        case BNH_CPL_LONG_INT:
            wdmcpl_create_xgc_field_adapter_t<long>(name, comm, data, size, *rcn, min_class, max_class, *adpt_h->adpt);
        default:
            std::cerr << "ERROR: bad data type in " << __func__ << std::endl;
            return(NULL);
    }
   
    app_h->adpts[name] = std::shared_ptr<struct field_adapter>(adpt_h);
    return(adpt_h);
}

extern "C" struct field_adapter *create_dummy_adaptor()
{
    struct field_adapter *adpt_h = new struct field_adapter;
    
    adpt_h->adpt = new AdapterVariant{};
    adpt_h->adpt->emplace<wdmcpl::DummyFieldAdapter>();

    return(adpt_h);
}

enum field_type {PCMS_FIELD_COUPLED, PCMS_FIELD_CONVERTIBLE};
struct field_handle {
    union {
        wdmcpl::CoupledField *cpl_field;
        wdmcpl::ConvertibleCoupledField *conv_field;
    };
    enum field_type type;
};

struct AddClientFieldVariantOperators {
    AddClientFieldVariantOperators(const char* name, wdmcpl::CouplerClient* client, int participates)
    : name_(name), client_(client), participates_(participates)
    {
    }

    [[nodiscard]]
    wdmcpl::CoupledField* operator()(const std::monostate&) const noexcept { return nullptr; }
    template <typename FieldAdapter>
    [[nodiscard]]
    wdmcpl::CoupledField* operator()(const FieldAdapter& field_adapter) const noexcept {
        return client_->AddField(name_, field_adapter, participates_);
    }

    const char* name_;
    wdmcpl::CouplerClient* client_;
    bool participates_;
};

struct AddServerFieldVariantOperators {
    AddServerFieldVariantOperators(const char *name, wdmcpl::Application *app, 
                                      Omega_h::Read<Omega_h::I8> &internal_field_mask)
    : name_(name), app_(app), internal_field_mask_(internal_field_mask)
    {
    }

    wdmcpl::ConvertibleCoupledField *operator()(const std::monostate&) const noexcept { return nullptr; }
    template <typename FieldAdapter>
    wdmcpl::ConvertibleCoupledField *operator()(const FieldAdapter& field_adapter) const noexcept {
        return app_->AddField(name_, field_adapter, wdmcpl::FieldTransferMethod::Copy, wdmcpl::FieldEvaluationMethod::None, wdmcpl::FieldTransferMethod::Copy, wdmcpl::FieldEvaluationMethod::None, internal_field_mask_);
    }
    const char *name_;
    wdmcpl::Application *app_;
    Omega_h::Read<Omega_h::I8> internal_field_mask_;
};

extern "C" struct field_handle *cpl_add_field(struct cpl_hndl *cpl_h, const char *app_name, const char *name, int participates)
{
    struct app_hndl *app_h = cpl_h->apps[std::string(app_name)].get();
    wdmcpl::Application *app = app_h->handle;
    struct field_adapter *adpt_h = app_h->adpts[std::string(name)].get();
    struct field_handle *field = new struct field_handle;

    if(cpl_h->server) {
        ServerAdapterVariant *adpt = adpt_h->srv_adpt;
        field->type = PCMS_FIELD_CONVERTIBLE;
        field->conv_field = std::visit(AddServerFieldVariantOperators{name, app, *app_h->overlap_h}, *adpt);
    } else {
        auto *cpl = cpl_h->cpl_client;
        AdapterVariant *adpt = adpt_h->adpt;
        field->type = PCMS_FIELD_COUPLED;
        field->cpl_field = std::visit(AddClientFieldVariantOperators{name, cpl, participates}, *adpt);
    }

    return(field);
}

extern "C" void cpl_recv_field(struct field_handle *field_hndl)
{
    if(field_hndl->type == PCMS_FIELD_CONVERTIBLE) {
        auto field = field_hndl->conv_field;
        field->Receive();
    } else if(field_hndl->type == PCMS_FIELD_COUPLED) {
        auto field = field_hndl->cpl_field;
        field->Receive();
    } else {
        std::cerr << "ERROR: unknown field type in " << __func__ << std::endl;
    }
}

extern "C" void cpl_send_field(struct field_handle *field_hndl)
{
    if(field_hndl->type == PCMS_FIELD_CONVERTIBLE) {
        auto field = field_hndl->conv_field;
        field->Send();
    } else if(field_hndl->type == PCMS_FIELD_COUPLED) {
        auto field = field_hndl->cpl_field;
        field->Send();
    } else {
        std::cerr << "ERROR: unknown field type in " << __func__ << std::endl;
    }
}

/*
extern "C" struct cpl_gid_field *create_gid_field(struct app_hndl *apph, const char *field_name, struct cpl_hndl *cphp, struct omegah_mesh *meshp, void *field_buf)
{
    wdmcpl::Application *app = apph->handle;
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)get_mesh(meshp); 
    //auto &app = cpl_h->AddApplication(app_name);
    struct cpl_gid_field *field = new struct cpl_gid_field();

    field->field_name = strdup(app_name);
    field->cpl = cphp;

    if(cphp->server) {
        auto cpl = (wdmcpl::CouplerServer *)(cphp->cpl_srv);
        field->field = cpl->AddField(app_name, wdmcpl::OmegaHFieldAdapter<wdmcpl::GO>(app_name, *mesh, *cphp->overlap_h), wdmcpl::FieldTransferMethod::Copy,
               wdmcpl::FieldEvaluationMethod::None,
               wdmcpl::FieldTransferMethod::Copy,
               wdmcpl::FieldEvaluationMethod::None, *cphp->overlap_h);
        cphp->fields_.insert(std::pair<std::string, std::reference_wrapper<wdmcpl::ConvertibleCoupledField>>(app_name, *field->field));
        //wdmcpl::InternalField intf = field->field->GetInternalField(); 
        cphp->internal_fields_.insert(std::pair<std::string, wdmcpl::InternalField>(std::string(app_name), wdmcpl::InternalField(field->field->GetInternalField())));
    } else {
        auto cpl = (wdmcpl::CouplerClient *)(cphp->cpl_client);
        cpl->AddField(app_name,  wdmcpl::OmegaHFieldAdapter<wdmcpl::GO>("global", *mesh, *cphp->overlap_h));
    }

    return(field);
}

extern "C" void cpl_combine_fields(struct cpl_hndl *cphp, int num_fields, const char **field_names)
{
    std::vector<std::string> fields_to_combine;
    std::vector<std::reference_wrapper<wdmcpl::InternalField>> combine_fields;
    auto combiner = MeanCombiner{};
    int i;

    combine_fields.reserve(num_fields);
    for(i = 0; i < num_fields; i++) {
        wdmcpl::ConvertibleCoupledField& field = cphp->fields_.at(std::string(field_names[i]));
        //combine_fields.insert(std::pair<std::string, std::reference_wrapper<wdmcpl::InternalField>>(std::string(field_names[i]), field.GetInternalField()));
        combine_fields.push_back(field.GetInternalField());
    }
    auto& combined = wdmcpl::detail::find_or_create_internal_field<wdmcpl::Real>(
      std::string("combined_gids"), cphp->internal_fields_, *cphp->mesh, *cphp->overlap_h);
    combiner(combine_fields, combined);

}
extern "C" void *cpl_get_field_ptr(struct cpl_gid_field *field)
{
    return(field->field);
}

extern "C" void cpl_send_field(struct cpl_gid_field *field)
{
    struct cpl_hndl *cplh = field->cpl;
    field->tsendstart.push_back(std::chrono::steady_clock::now());
    APEX_NAME_TIMER_START(1, "field_send");
    if(cplh->server) {
        wdmcpl::ConvertibleCoupledField *coupled_field = field->field;
        coupled_field->SyncInternalToNative();
        coupled_field->Send();
    } else {
        wdmcpl::CouplerClient *cpl = cplh->cpl_client;
        cpl->SendField(field->field_name);
    }
    APEX_TIMER_STOP(1);
    field->tsendend.push_back(std::chrono::steady_clock::now());
}

extern "C" void cpl_recv_field(struct cpl_gid_field *field, double **buffer, size_t *num_elem)
{
    struct cpl_hndl *cplh = field->cpl;
    field->trecvstart.push_back(std::chrono::steady_clock::now());
    APEX_NAME_TIMER_START(1, "field_recv");
    if(cplh->server) {
        wdmcpl::ConvertibleCoupledField *coupled_field = field->field;
        coupled_field->Receive();
        coupled_field->SyncNativeToInternal();
    } else {
        wdmcpl::CouplerClient *cpl = cplh->cpl_client;
        cpl->ReceiveField(field->field_name);
    }
    APEX_TIMER_STOP(1);
    field->trecvend.push_back(std::chrono::steady_clock::now());
}
*/

extern "C" void report_send_recv_timing(struct cpl_gid_field *field, const char *name)
{
    int i, rank;
    char timer_str[100];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(i = 0; i < field->tsendstart.size(); i++) {
        sprintf(timer_str, "%sSend%i", name, i);
        comparePrintTime(field->tsendstart[i], field->tsendend[i], timer_str, rank); 
    }

    for(i = 0; i < field->trecvstart.size(); i++) {
        sprintf(timer_str, "%sRecv%i", name, i);
        comparePrintTime(field->trecvstart[i], field->trecvend[i], timer_str, rank);
    }
}

#endif
