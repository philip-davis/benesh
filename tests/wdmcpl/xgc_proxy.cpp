#include<sstream>
#include<iostream>
#include<cstdint>
#include<cstring>
#include<chrono>

#include<mpi.h>
#include <Kokkos_Core.hpp>

#include "span.h"

#include "benesh.h"

#ifdef USE_APEX
#include<apex.h>
#define APEX_FUNC_TIMER_START(fn) \
        apex_profiler_handle profiler0 = apex_start(APEX_FUNCTION_ADDRESS, &fn);
#define APEX_NAME_TIMER_START(num, name) \
        apex_profiler_handle profiler##num = apex_start(APEX_NAME_STRING, name);
#define APEX_TIMER_STOP(num) \
        apex_stop(profiler##num);
#else
#define APEX_TIMER_STOP(num) (void)0
#define APEX_NAME_TIMER_START(num, name) (void)0
#define APEX_FUNC_TIMER_START(fn) (void)0
#endif



int do_field_transfer(benesh_app_id bnh, benesh_arg arg)
{
    benesh_unify_mesh_data(bnh, "data");
    //

    return(0);
}

void timeMinMaxAvg(double time, double& min, double& max, double& avg) {
  const auto comm = MPI_COMM_WORLD;
  int nproc;
  MPI_Comm_size(comm, &nproc);
  double tot = 0;
  MPI_Allreduce(&time, &min, 1, MPI_DOUBLE, MPI_MIN, comm);
  MPI_Allreduce(&time, &max, 1, MPI_DOUBLE, MPI_MAX, comm);
  MPI_Allreduce(&time, &tot, 1, MPI_DOUBLE, MPI_SUM, comm);
  avg = tot / nproc;
}

void printTime(std::string_view mode, double min, double max, double avg) {
  std::stringstream ss;
  ss << mode << " elapsed time min, max, avg (s): "
   << min << " " << max << " " << avg << "\n";
  std::cout << ss.str();
}

template <class T> void getAndPrintTime(T start, std::string_view key, int rank)
{
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    double min, max, avg;
    timeMinMaxAvg(elapsed_seconds.count(), min, max, avg);
    if(!rank)
        printTime(key, min, max, avg);
}

int b_gen_data(benesh_app_id bnh, void *arg)
{
    return(0);
}

int b_field_transfer(benesh_app_id bnh, void *arg)
{
    return(0);
}

nonstd::span<uint64_t> bind_data(benesh_app_id bnh, const char *meshFileName, const char *cpnFileName)
{
    char *dom_name;
    uint64_t *var_buf;
    uint64_t buf_size;

    benesh_get_var_domain(bnh, "data", &dom_name, NULL, NULL, NULL);
    benesh_bind_mesh_domain(bnh, dom_name, meshFileName, cpnFileName, 1);
    var_buf = (uint64_t *)benesh_get_var_buf(bnh, "data", &buf_size);
    const size_t N = buf_size / sizeof(uint64_t);

    return(nonstd::span<uint64_t>(var_buf, N));
}

void client(const char *meshFileName, int clientId, const char *cpnFileName)
{
    benesh_app_id bnh;
    std::stringstream ss;
    char *dom_name;
    char timer_str[100];
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    ss << "client" << clientId;
    
    benesh_init(ss.str().c_str(), "omegah.xc", MPI_COMM_WORLD, 1, &bnh);
    nonstd::span<uint64_t> msg = bind_data(bnh, meshFileName, cpnFileName);
    benesh_bind_method(bnh, "gen_data", b_gen_data, NULL);
    benesh_bind_method(bnh, "field_transfer", b_field_transfer, NULL);

    APEX_NAME_TIMER_START(1, "client_main_work");
    for(int i = 0; i < 11; i++) {
        // set data in msg
        std::stringstream ss;
        ss << "step." << i;
        sprintf(timer_str, "client%i.SendRecv%i", clientId, i);
        auto start = std::chrono::steady_clock::now();
        benesh_tpoint(bnh, ss.str().c_str());
//        getAndPrintTime(start,timer_str,rank);
    }
    APEX_TIMER_STOP(1);

    benesh_fini(bnh);
}

void server(const char *meshFileName, const char *cpnFileName)
{
    benesh_app_id bnh;
    char timer_str[100];

    benesh_init("coupler", "omegah.xc", MPI_COMM_WORLD, 1, &bnh);
    nonstd::span<uint64_t> msg = bind_data(bnh, meshFileName, cpnFileName);
    benesh_bind_method(bnh, "field_transfer", do_field_transfer, NULL);
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   
    APEX_NAME_TIMER_START(1, "coupler_main_work");
    for(int i = 1; i < 10; i++) {
        std::stringstream ss;
        ss << "step." << i;
        sprintf(timer_str, "serverSendRecv%i", i);
        auto start = std::chrono::steady_clock::now();
        benesh_tpoint(bnh, ss.str().c_str());
       // getAndPrintTime(start,timer_str, rank);
        // check data in msg
    }
    APEX_TIMER_STOP(1);

    benesh_fini(bnh);
}

int main(int argc, char** argv)
{
    int rank, size;
    int provided;

    Kokkos::ScopeGuard kokkos{};
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    //MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(argc != 4) {
        if(!rank) {
            std::cerr << "Usage: " << argv[0] << " <clientId=0|1|2> /path/to/omega_h/mesh /path/to/partitionFile.cpn\n";
        }
        exit(-1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 
#ifdef USE_APEX
    apex_init("xgc couple", rank, size);
#endif


    const auto clientId = atoi(argv[1]);
    const auto meshFileName = argv[2];
    const auto cpnFileName = strcmp(argv[3], "ignored") == 0 ? (const char *)NULL : argv[3];

    if(clientId) {
        client(meshFileName, clientId, cpnFileName);
    } else {
        server(meshFileName, cpnFileName);
    }

#ifdef USE_APEX
    apex_finalize();
#endif
    MPI_Finalize();

    return(0);
}
