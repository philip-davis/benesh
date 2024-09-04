#ifndef _BENESH_COMM_H
#define _BENESH_COMM_H

#include <mpi.h>

struct benesh_comm;

enum bnh_dummy_cmd_type { BNH_DCMD_DATA, BNH_DCMD_TERM };

typedef int (*dcmd_cb)(int id, void *arg, void *payload);

struct benesh_dummy_cmd_handler {
    int type;
    int id;
    void *saved_arg;
    void *payload;
};

int benesh_dummy_cmd_listen(struct benesh_comm *bcomm);

int benesh_dummy_term(struct benesh_comm *bcomm);

struct benesh_comm *benesh_comm_init(MPI_Comm gcomm, int f_dummy);

int benesh_comm_get_my_comm(struct benesh_comm *bcomm, MPI_Comm *comm);

int benesh_comm_app_barrier(struct benesh_comm *bcomm);

#endif // _BENESH_COMM_H