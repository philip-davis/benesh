#include "benesh-tasks.h"
#include "benesh-logging.h"
#include "benesh-targets.h"
#include "benesh.h"

#include <abt.h>

enum bnh_task_type {
    BNH_TASK_IMPORT,
};

struct benesh_task {
    enum bnh_task_type type;
    struct benesh_task *prev;
    struct benesh_task *next;
    struct benesh_rule *rule;
    int directive_id;
    int f_announce;
};

struct benesh_taskman {
    struct benesh_task *queue_head;
    struct benesh_task *queue_tail;
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
               "signal failed with %i\n", err);

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

int benesh_enqueue(struct benesh_taskman *btm, struct benesh_task *task)
{
    TRACE_OUT;
    int err, err2;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad task manager handle.\n");
    }
    if(!task) {
        ERR_OUT(BNH_EFAULT, err_out, "bad task.\n");
    }

    CHECK_ZERO(benesh_taskman_lock(btm), err, err_out,
               "failed to lock task manager.\n");
    task->next = btm->queue_head;
    if(btm->queue_head) {
        btm->queue_head->prev = task->next;
    }
    btm->queue_head = task;
    if(!btm->queue_tail) {
        btm->queue_tail = task;
    }

    CHECK_ZERO(benesh_taskman_signal(btm), err, err_out_unlock,
               "failed to signal waiting thraads.\n");
    CHECK_ZERO(benesh_taskman_unlock(btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");

    return (0);
err_out_unlock:
    // successful unlock will clear err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_taskman_unlock(btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");
    err = err2;
err_out:
    return (err);
}

int benesh_enqueue_import(struct benesh_taskman *btm, struct benesh_rule *rule,
                          int directive_id, int64_t *tgt_vars)
{
    TRACE_OUT;
    struct benesh_task *import;
    size_t nvar;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad taskman handle.\n");
    }

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n")
    }

    if(directive_id < 0 || directive_id >= benesh_get_num_directives(rule)) {
        ERR_OUT(BNH_EINVAL, err_out, "bad directive id %i.\n", directive_id);
    }

    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), err, err_out,
               "could not access rule.\n");
    if(nvar && !tgt_vars) {
        ERR_OUT(BNH_EFAULT, err_out, "missing mapped values.\n");
    }

    ASSIGN_NOT_NULL(calloc(sizeof(*import), 1), import, BNH_ENOMEM, err_out,
                    "memory allocation failure.\n");
    import->type = BNH_TASK_IMPORT;
    import->rule = rule;
    import->directive_id = directive_id;

    CHECK_ZERO(benesh_enqueue(btm, import), err, err_free,
               "could not enqueue.\n");

    return (0);
err_free:
    free(import);
err_out:
    return (err);
}