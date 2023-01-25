#ifndef __BNH_CPL_WRAPPER_H_
#define __BNH_CPL_WRAPPER_H_

#include<mpi.h>

#include<inttypes.h>
#include<vector>
#include<chrono>

#include<wdmcpl/wdmcpl.h>

#include <Omega_h_mesh.hpp>

#include "omegah_wrapper.h"
#include "redev_wrapper.h"

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

struct cpl_hndl {
    union {
        wdmcpl::CouplerServer *cpl_srv;
        wdmcpl::CouplerClient *cpl_client;
    };
    bool server;
    union {
       Omega_h::Read<Omega_h::I8> *overlap_h;
       Omega_h::Read<Omega_h::I8> *srv_overlap_h; 
    };
    int64_t *buffer;
    size_t num_elem;
};

struct cpl_gid_field {
    /*
    wdmcpl::FieldCommunicatorT<wdmcpl::GO> *comm;
    std::vector<wdmcpl::GO> gid_field;
    */
    ConvertibleCoupledField *field;
    char *field_name;
    struct cpl_hndl *cpl;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> tsendstart;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> tsendend;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> trecvstart;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> trecvend;
};

extern "C" struct cpl_hndl *create_cpl_hndl(const char *wfname, struct omegah_mesh *meshp, struct rdv_ptn *ptnp, int server)
{
    auto ptn = (redev::ClassPtn *)ptnp;
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)get_mesh(meshp);
    struct cpl_hndl *cpl_h = (struct cpl_hndl *)malloc(sizeof(*cpl_h));
    cpl_h->server = (bool)server;
    if(cpl_h->server) {
        cpl_h->cpl_srv = new wdmcpl::CouplerServer(wfname, MPI_COMM_WORLD, *ptn, *mesh);
    } else {
        cpl_h->cpl_client = new wdmcpl::CouplerClient(wfname, MPI_COMM_WORLD);
    }
    return(cpl_h);
}

extern "C" void close_cpl(struct cpl_hndl *cpl_h)
{
    if(cpl_h->server) {
        delete(cpl_h->cpl_srv);
    } else {
        delete(cpl_h->cpl_client);
    }
}

extern "C" void mark_cpl_overlap(struct cpl_hndl *cph, struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class)
{
    if(cph->server) {
        cph->srv_overlap_h = (Omega_h::Read<Omega_h::I8> *)mark_server_mesh_overlap(meshp, rptn, min_class, max_class);
    } else {
        cph->overlap_h = (Omega_h::Read<Omega_h::I8> *)mark_mesh_overlap(meshp, min_class, max_class);
    }
}

extern "C" struct cpl_gid_field *create_gid_field(const char *app_name, const char *field_name, struct cpl_hndl *cphp, struct omegah_mesh *meshp, void *field_buf)
{
    Omega_h::Mesh *mesh = (Omega_h::Mesh *)get_mesh(meshp); 
    //auto &app = cpl_h->AddApplication(app_name);
    struct cpl_gid_field *field = new struct cpl_gid_field();

    field->field_name = strdup(app_name);
    field->cpl = cphp;

    if(cphp->server) {
        auto cpl = (wdmcpl::CouplerServer *)(cphp->cpl_srv);
        field->field = cpl->AddField(app_name, wdmcpl::OmegaHFieldAdapter<wdmcpl::GO>(app_name, *mesh, *cphp->srv_overlap_h), wdmcpl::FieldTransferMethod::Copy,
               wdmcpl::FieldEvaluationMethod::None,
               wdmcpl::FieldTransferMethod::Copy,
               wdmcpl::FieldEvaluationMethod::None, *cphp->srv_overlap_h);
    } else {
        auto cpl = (wdmcpl::CouplerClient *)(cphp->cpl_client);
        cpl->AddField(app_name,  wdmcpl::OmegaHFieldAdapter<wdmcpl::GO>("global", *mesh, *cphp->overlap_h));
    }

    return(field);
}

extern "C" void cpl_send_field(struct cpl_gid_field *field)
{
    struct cpl_hndl *cplh = field->cpl;
    field->tsendstart.push_back(std::chrono::steady_clock::now());
    APEX_NAME_TIMER_START(1, "field_send");
    if(cplh->server) {
        wdmcpl::CouplerServer *cpl = cplh->cpl_srv;
        cpl->SendField(field->field_name);
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
        wdmcpl::CouplerServer *cpl = cplh->cpl_srv;
        cpl->ReceiveField(field->field_name);
    } else {
        wdmcpl::CouplerClient *cpl = cplh->cpl_client;
        cpl->ReceiveField(field->field_name);
    }
    APEX_TIMER_STOP(1);
    field->trecvend.push_back(std::chrono::steady_clock::now());
}

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
