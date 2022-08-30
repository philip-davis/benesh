#include<sstream>
#include<iostream>
#include<cstdint>
#include<cstring>
#include<sys/prctl.h>

#include<mpi.h>

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

    ss << "client" << clientId;
    
    benesh_init(ss.str().c_str(), "omegah.xc", MPI_COMM_WORLD, 1, &bnh);
    nonstd::span<uint64_t> msg = bind_data(bnh, meshFileName, cpnFileName);

    for(int i = 0; i < 3; i++) {
        // set data in msg
        std::stringstream ss;
        ss << "step." << i;
        benesh_tpoint(bnh, ss.str().c_str());
    }

    benesh_fini(bnh);
}

void server(const char *meshFileName, const char *cpnFileName)
{
    benesh_app_id bnh;

    benesh_init("coupler", "omegah.xc", MPI_COMM_WORLD, 1, &bnh);
    nonstd::span<uint64_t> msg = bind_data(bnh, meshFileName, cpnFileName);

    for(int i = 0; i < 3; i++) {
        std::stringstream ss;
        ss << "step." << i;
        benesh_tpoint(bnh, ss.str().c_str());
        // check data in msg
    }

    benesh_fini(bnh);
}

int main(int argc, char** argv)
{
    int rank, size;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(argc != 4) {
        if(!rank) {
            std::cerr << "Usage: " << argv[0] << " <clientId=0|1|2> /path/to/omega_h/mesh /path/to/partitionFile.cpn\n";
        }
        exit(-1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 
    prctl(PR_SET_THP_DISABLE, 1, 0, 0, 0);

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
