#include "benesh-comm.h"
#include "benesh-logging.h"
#include "benesh.h"

#include <mpi.h>
#include <stdlib.h>

struct benesh_dummy_cmd {
    int32_t type_id;
    int32_t subtype_id;
    uint32_t payload_sz;
    void *payload;
};

struct benesh_comm {
    int rank;
    int grank;
    int comm_size;
    MPI_Comm my_comm;
    MPI_Comm gcomm;
    int root_rank;
    int root_drank;
};

static int benesh_dummy_handle_command(struct benesh_comm *bcomm,
                                       struct benesh_dummy_cmd *cmd,
                                       int *f_terminate)
{
    TRACE_OUT;
    int err;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh communicator.\n");
    }
    if(!cmd) {
        ERR_OUT(BNH_EFAULT, err_out, "bad command.\n");
    }
    if(!f_terminate) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    if(cmd->type_id == BNH_DCMD_TERM) {
        *f_terminate = 1;
        return (0);
    }
    ERR_OUT(BNH_ENOSYS, err_out, "dummy command handling not yet implemented.");

    return (0);
err_out:
    *f_terminate = 1;
    return (err);
}

int benesh_dummy_cmd_listen(struct benesh_comm *bcomm)
{
    TRACE_OUT;
    int err;
    struct benesh_dummy_cmd cmd = {0};
    int term = 0;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh communicator.\n");
    }

    do {
        if(bcomm->grank == bcomm->root_drank) {
            CHECK_ZERO(MPI_Recv(&cmd, sizeof(cmd), MPI_BYTE, bcomm->root_rank,
                                0, bcomm->gcomm, MPI_STATUS_IGNORE),
                       BNH_EIO, err_out, "failure receiving command.\n");
            if(cmd.payload_sz) {
                ASSIGN_NOT_NULL(malloc(cmd.payload_sz), cmd.payload, BNH_ENOMEM,
                                err_out, "buffer allocation failure.\n");
                CHECK_ZERO(MPI_Recv(cmd.payload, cmd.payload_sz, MPI_BYTE,
                                    bcomm->root_rank, 0, bcomm->gcomm,
                                    MPI_STATUS_IGNORE),
                           BNH_EIO, err_out,
                           "failure receiving command payload.\n");
            }
        }
        CHECK_ZERO(MPI_Bcast(&cmd, sizeof(cmd), MPI_BYTE, 0, bcomm->my_comm),
                   BNH_EIO, err_out, "failure broadcasting command.\n");
        if(cmd.payload_sz) {
            if(bcomm->grank != bcomm->root_drank) {
                ASSIGN_NOT_NULL(malloc(cmd.payload_sz), cmd.payload, BNH_ENOMEM,
                                err_out, "buffer allocation failure.\n");
            }
            CHECK_ZERO(MPI_Bcast(&cmd.payload, cmd.payload_sz, MPI_BYTE, 0,
                                 bcomm->my_comm),
                       BNH_EIO, err_out, "failure broadcasting command.\n");
        }
        CHECK_ZERO(benesh_dummy_handle_command(bcomm, &cmd, &term), err,
                   err_out, "command handling failure.\n");
        if(cmd.payload) {
            free(cmd.payload);
        }
    } while(!term);

    return (0);
err_out:
    return (err);
}

int benesh_dummy_send_cmd(struct benesh_comm *bcomm,
                          struct benesh_dummy_cmd *cmd)
{
    TRACE_OUT;
    int err;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad communicator handle.\n");
    }
    if(!cmd) {
        ERR_OUT(BNH_EFAULT, err_out, "bad command.\n");
    }

    CHECK_ZERO(MPI_Send(cmd, sizeof(*cmd), MPI_BYTE, bcomm->root_drank, 0,
                        bcomm->gcomm),
               BNH_EINVAL, err_out, "bad communicator");
    if(cmd->payload_sz) {
        CHECK_ZERO(MPI_Send(cmd->payload, cmd->payload_sz, MPI_BYTE,
                            bcomm->root_drank, 0, bcomm->gcomm),
                   BNH_EINVAL, err_out, "bad communicator");
    }

    return (0);
err_out:
    return (err);
}

int benesh_dummy_term(struct benesh_comm *bcomm)
{
    TRACE_OUT;
    struct benesh_dummy_cmd cmd = {0};
    int err;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad communicator handle.\n");
    }

    if(!bcomm->rank && bcomm->root_drank > -1) {
        cmd.type_id = BNH_DCMD_TERM;
        CHECK_ZERO(benesh_dummy_send_cmd(bcomm, &cmd), err, err_out,
                   "could not send term command.\n");
    }

    return (0);
err_out:
    return (err);
}

struct benesh_comm *benesh_comm_init(MPI_Comm gcomm, int f_dummy)
{
    TRACE_OUT;
    struct benesh_comm *bcomm;
    int err;

    ASSIGN_NOT_NULL(malloc(sizeof(*bcomm)), bcomm, BNH_ENOMEM, err_out,
                    "failed to assign buffer.\n");
    MPI_Comm_dup(gcomm, &bcomm->gcomm);
    MPI_Comm_rank(gcomm, &bcomm->grank);
    MPI_Comm_split(gcomm, f_dummy, bcomm->grank, &bcomm->my_comm);
    MPI_Comm_rank(bcomm->my_comm, &bcomm->rank);
    MPI_Comm_size(bcomm->my_comm, &bcomm->comm_size);
    bcomm->root_rank = bcomm->root_drank = -1;
    if(bcomm->rank == 0) {
        if(f_dummy) {
            bcomm->root_drank = bcomm->grank;
        } else {
            bcomm->root_rank = bcomm->grank;
        }
    }

    MPI_Allreduce(MPI_IN_PLACE, &bcomm->root_drank, 1, MPI_INT, MPI_MAX,
                  bcomm->gcomm);
    MPI_Allreduce(MPI_IN_PLACE, &bcomm->root_rank, 1, MPI_INT, MPI_MAX,
                  bcomm->gcomm);

    return (bcomm);
err_out:
    return (NULL);
}

int benesh_comm_get_my_comm(struct benesh_comm *bcomm, MPI_Comm *comm)
{
    TRACE_OUT;
    int err;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad communicator handle.\n");
    }
    if(!comm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    *comm = bcomm->my_comm;

    return (0);
err_out:
    return (err);
}

int benesh_comm_app_barrier(struct benesh_comm *bcomm)
{
    TRACE_OUT;
    int err;

    if(!bcomm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad communicator handle.\n");
    }

    CHECK_ZERO(MPI_Barrier(bcomm->gcomm), BNH_EIO, err_out,
               "failure in barrier.\n");

    return (0);
err_out:
    return (err);
}