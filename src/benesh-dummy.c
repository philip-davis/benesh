#include "benesh-dummy.h"
#include "benesh-core-types.h"
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

static int benesh_dummy_handle_command(struct benesh_handle *bnh,
                                       struct benesh_dummy_cmd *cmd,
                                       int *f_terminate)
{
    TRACE_OUT;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
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

int benesh_dummy_cmd_listen(struct benesh_handle *bnh)
{
    TRACE_OUT;
    int err;
    struct benesh_dummy_cmd cmd;
    int term = 0;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    do {
        if(bnh->grank == bnh->root_drank) {
            CHECK_ZERO(MPI_Recv(&cmd, sizeof(cmd), MPI_BYTE, bnh->root_rank, 0,
                                bnh->gcomm, MPI_STATUS_IGNORE),
                       BNH_EIO, err_out, "failure receiving command.\n");
            if(cmd.payload) {
                CHECK_ZERO(
                    MPI_Recv(&cmd.payload, cmd.payload_sz, MPI_BYTE,
                             bnh->root_rank, 0, bnh->gcomm, MPI_STATUS_IGNORE),
                    BNH_EIO, err_out, "failure receiving command payload.\n");
            }
        }
        CHECK_ZERO(MPI_Bcast(&cmd, sizeof(cmd), MPI_BYTE, 0, bnh->my_comm),
                   BNH_EIO, err_out, "failure broadcasting command.\n");
        if(cmd.payload) {
            CHECK_ZERO(MPI_Bcast(&cmd.payload, cmd.payload_sz, MPI_BYTE, 0,
                                 bnh->my_comm),
                       BNH_EIO, err_out, "failure broadcasting command.\n");
        }
        CHECK_ZERO(benesh_dummy_handle_command(bnh, &cmd, &term), err, err_out,
                   "command handling failure.\n");
    } while(!term);

    return (0);
err_out:
    return (err);
}