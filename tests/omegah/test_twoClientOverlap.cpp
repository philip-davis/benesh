#include<sstream>
#include<iostream>

#include<mpi.h>

#include "benesh.h"

void client(const char *meshFileName, int clientId)
{
    benesh_app_id bnh;
    std::stringstream ss;

    ss << "client" << clientId;
    
    benesh_init(ss.str().c_str(), "omegah.xc", MPI_COMM_WORLD, 1, &bnh);

}

void server(const char *meshFileName, const char *cpnFileName)
{
    benesh_app_id bnh;

    benesh_init("coupler", "omegah.xc", MPI_COMM_WORLD, 1, &bnh);
}

int main(int argc, char** argv)
{
    int rank;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(argc != 4) {
        if(!rank) {
            std::cerr << "Usage: " << argv[0] << " <clientId=0|1|2> /path/to/omega_h/mesh /path/to/partitionFile.cpn\n";
        }
        exit(-1);
    }

    const auto clientId = atoi(argv[1]);
    const auto meshFileName = argv[2];
    const auto cpnFileName = argv[3];

    if(clientId) {
        client(meshFileName, clientId);
    } else {
        server(meshFileName, cpnFileName);
    }

    MPI_Finalize();

    return(0);
}
