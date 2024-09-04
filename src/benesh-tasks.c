#include "benesh-tasks.h"
#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-targets.h"
#include "benesh.h"
#include "beneshp.h"

#include <abt.h>
#include <stdatomic.h>

struct benesh_task {
    enum bnh_task_type type;
    struct benesh_task *prev;
    struct benesh_task *next;
    struct benesh_target *tgt;
    int directive_id;
    atomic_int remaining_prereqs;
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

static int benesh_task_enqueue_hook(struct benesh_task *task)
{
    TRACE_OUT;
    int err;

    if(!task) {
        ERR_OUT(BNH_EFAULT, err_out, "bad task.\n");
    }

    // TODO: type-specific setup wrok at enqueue time (e.g. data subscriotions)

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

    CHECK_ZERO(benesh_task_enqueue_hook(task), err, err_out_unlock,
               "an enqueue callback faileed.\n");

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

static struct benesh_task *benesh_task_import(struct benesh_target *tgt,
                                              int directive_id)
{
    TRACE_OUT;
    size_t ndir;
    struct benesh_task *import;
    int err;

    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    CHECK_ZERO(benesh_target_get_ndir(tgt, &ndir), err, err_out,
               "could not access target.\n");
    if(directive_id < 0 || directive_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "directive id %i is out of range.\n",
                directive_id);
    }
    ASSIGN_NOT_NULL(calloc(1, sizeof(*import)), import, BNH_ENOMEM, err_out,
                    "memory allocation failure.\n");
    import->type = BNH_TASK_IMPORT;
    import->tgt = tgt;
    import->directive_id = directive_id;

    return (import);
err_out:
    return (NULL);
}

int benesh_enqueue_import(struct benesh_taskman *btm, struct benesh_target *tgt,
                          int directive_id)
{
    TRACE_OUT;
    struct benesh_task *import;
    size_t ndir;
    int err;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad taskman handle.\n");
    }
    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n")
    }
    CHECK_ZERO(benesh_target_get_ndir(tgt, &ndir), err, err_out,
               "could not access target.\n");
    if(directive_id < 0 || directive_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "directive id %i is out of range.\n",
                directive_id);
    }

    ASSIGN_NOT_NULL(benesh_task_import(tgt, directive_id), import, BNH_ENOMEM,
                    err_out, "failed to create import task.\n");
    CHECK_ZERO(benesh_enqueue(btm, import), err, err_free,
               "could not enqueue.\n");

    return (0);
err_free:
    free(import);
err_out:
    return (err);
}

static struct benesh_task *benesh_task_init(struct benesh_handle *bnh,
                                            struct benesh_target *tgt)
{
    TRACE_OUT;
    struct benesh_task *task;
    struct bnh_pvec *tgt_prereqs;
    struct benesh_target *prereq;
    bnh_pvec_iter bi;
    int nprereq;
    int status;
    int err;

    task = NULL;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*task)), task, BNH_ENOMEM, err_out,
                    "failure allocating buffer.\n");
    task->type = BNH_TASK_START_OBJ;
    task->tgt = tgt;
    task->remaining_prereqs = 0;

    ASSIGN_NOT_NULL(benesh_target_get_prereqs(bnh, tgt), tgt_prereqs,
                    BNH_ENOENT, err_out, "failed to get target prereqs.\n");
    nprereq = bnh_pvec_get_len(tgt_prereqs);
    BNH_PVEC_FOREACH(prereq, bi, tgt_prereqs)
    {
        CHECK_ZERO(benesh_target_get_status(tgt, &status), BNH_ESTATE, err_out,
                   "cannot access target.\n");
        if(status != BNH_STAT_REAL) {
            CHECK_ZERO(benesh_target_sub_task(prereq, task), err, err_out,
                       "could not subscribe task to prereq target.\n");
            task->remaining_prereqs++;
            CHECK_ZERO(benesh_taskman_schedule_target(bnh, prereq), err,
                       err_out, "could not schedule prereq target.\n");
        }
    }

    return (task);
err_out:
    if(task)
        free(task);
    return (NULL);
}

static struct benesh_task *benesh_task_directive(struct benesh_handle *bnh,
                                                 struct benesh_target *tgt,
                                                 int dir_id)
{
    TRACE_OUT;
    struct benesh_task *task;
    struct bnh_pvec *tgt_prereqs;
    struct benesh_target *prereq;
    bnh_pvec_iter bi;
    size_t ndir;
    struct benesh_directive *dir;
    enum bnh_task_type dir_type;
    int nprereq;
    int status;
    int err;

    task = NULL;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    CHECK_ZERO(benesh_target_get_ndir(tgt, &ndir), BNH_ESTATE, err_out,
               "could not access target.\n");
    if(dir_id < 0 || dir_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "invalid directive id.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*task)), task, BNH_ENOMEM, err_out,
                    "failure allocating buffer.\n");

    ASSIGN_NOT_NULL(benesh_target_get_directive_by_id(tgt, dir_id), dir,
                    BNH_ENOENT, err_out, "could not retrieve directive.\n");
    CHECK_ZERO(benesh_directive_get_type(dir, &dir_type), err, err_out,
               "could not access directive.\n");
    task->type = dir_type;
    task->tgt = tgt;
    task->directive_id = dir_id;
    task->remaining_prereqs = 1;

    if(dir_id) {
        CHECK_ZERO(benesh_target_dir_status(tgt, dir_id - 1, &status),
                   BNH_ESTATE, err_out, "could not access directive.\n");
        if(status == BNH_DIR_STAT_DONE) {
            // the previous directive is done, and this task can go active.
            task->remaining_prereqs = 0;
        }
    } else {
        // first directive in the rule. This depends on the associated target
        // being started.
        CHECK_ZERO(benesh_target_get_status(tgt, &status), BNH_ESTATE, err_out,
                   "cannot access target.\n");
        if(status == BNH_STAT_INPROGRESS) {
            task->remaining_prereqs = 0;
        }
    }

    return (task);
err_out:
    if(task)
        free(task);
    return (NULL);
}

int benesh_taskman_schedule_directives(struct benesh_handle *bnh,
                                       struct benesh_target *tgt)
{
    TRACE_OUT;
    struct benesh_taskman *btm;
    struct bnh_pvec *directives;
    struct benesh_directive *dir;
    struct benesh_task *task;
    bnh_pvec_iter bi;
    int dir_id;
    int status;
    int has_local_work;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n")
    }
    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n")
    }

    ASSIGN_NOT_NULL(benesh_get_taskman(bnh), btm, BNH_EFAULT, err_out,
                    "bad task manager handle.\n");
    ASSIGN_NOT_NULL(benesh_target_get_directives(tgt), directives, BNH_EFAULT,
                    err_out, "bad target.\n");
    BNH_PVEC_FOREACH_ID(dir, dir_id, bi, directives)
    {
        if(!dir) {
            ERR_OUT(BNH_ESTATE, err_out, "bad directive.\n");
        }
        CHECK_ZERO(benesh_directive_is_local(bnh, dir, &has_local_work), err,
                   err_out, "could not check locality of directive.\n");
        if(has_local_work) {
            ASSIGN_NOT_NULL(benesh_task_directive(bnh, tgt, dir_id), task,
                            BNH_ENOMEM, err_out,
                            "could not create directive task.\n");
        }
        CHECK_ZERO(benesh_enqueue(btm, task), err, err_out,
                   "could not enqueue directive task.\n");
    }

    return (0);
err_out:
    return (err);
}

int benesh_taskman_schedule_target(struct benesh_handle *bnh,
                                   struct benesh_target *tgt)
{
    TRACE_OUT;
    struct benesh_taskman *btm;
    struct benesh_task *init_task;
    int status;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }

    ASSIGN_NOT_NULL(benesh_get_taskman(bnh), btm, BNH_EFAULT, err_out,
                    "bad task manager handle.\n");

    CHECK_ZERO(benesh_target_get_status(tgt, &status), err, err_out,
               "could not check target status.\n");
    /* target status can only be updated by the task manager, which is locked
     * during scheduling. */
    if(status == BNH_STAT_UNREAL) {
        ASSIGN_NOT_NULL(benesh_task_init(bnh, tgt), init_task, BNH_ENOMEM,
                        err_out,
                        "could not create target initialization task.\n");
        CHECK_ZERO(benesh_enqueue(btm, init_task), err, err_out,
                   "could not enqueue target initialization.\n");
        benesh_taskman_schedule_directives(bnh, tgt);
    }

    return (0);
err_out:
    return (err);
}

int benesh_taskman_queue_empty(struct benesh_taskman *btm, int *eout)
{
    TRACE_OUT;
    int err;
    *eout = 0;

    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad taskman handle.\n");
    }
    if(!eout) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    if(btm->queue_head) {
        return (0);
    }

    return (1);
err_out:
    *eout = err;
    return (1);
}