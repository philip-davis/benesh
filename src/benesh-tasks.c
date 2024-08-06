#include "benesh-tasks.h"
#include "benesh-logging.h"
#include "benesh.h"

#include <abt.h>

struct benesh_taskman {
    ABT_mutex mtx;
    ABT_cond cond;
};

int benesh_taskman_lock(struct benesh_taskman *btm)
{
    TRACE_OUT;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid task man handle.\n");
    }

    CHECK_ZERO(ABT_mutex_lock(btm->mtx), BNH_EABT, err_out,
               "mutex lock failed with %i\n", err);

    return (0);

err_out:
    return (err);
}

int benesh_taskman_unlock(struct benesh_taskman *btm)
{
    TRACE_OUT;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid task man handle.\n");
    }

    CHECK_ZERO(ABT_mutex_unlock(btm->mtx), BNH_EABT, err_out,
               "mutex unlock failed with %i\n", err);

    return (0);
err_out:
    return (err);
}

int benesh_taskman_signal(struct benesh_taskman *btm)
{
    TRACE_OUT;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid task man handle.\n");
    }

    CHECK_ZERO(ABT_cond_signal(btm->cond), BNH_EABT, err_out,
               "mutex signal failed with %i\n", err);

    return (0);
err_out:
    return (err);
}

int benesh_taskman_wait(struct benesh_taskman *btm)
{
    TRACE_OUT;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid task man handle.\n");
    }

    CHECK_ZERO(ABT_cond_wait(btm->cond, btm->mtx), BNH_EABT, err_out,
               "mutex wait failed with %i\n", err);

    return (0);

err_out:
    return (err);
}
