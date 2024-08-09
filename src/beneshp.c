#include "benesh-cohort.h"
#include "benesh-core-types.h"
#include "benesh-logging.h"
#include "benesh-obj.h"
#include "benesh-targets.h"
#include "benesh-tasks.h"
#include "util.h"

#include <unistd.h>

struct benesh_taskman *benesh_get_taskman(struct benesh_handle *bnh)
{
    TRACE_OUT;
    if(!bnh) {
        return (NULL);
    }
    return (bnh->btm);
}

int benesh_signal_taskman(struct benesh_handle *bnh)
{
    TRACE_OUT;
    int err, err2;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    CHECK_ZERO(benesh_taskman_lock(bnh->btm), err, err_out,
               "could not lock task manager.\n");
    CHECK_ZERO(benesh_taskman_signal(bnh->btm), err, err_out_lock,
               "could not signal task manager.\n");
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "could not unlock task manager. Possible deadlock!\n");

    return (0);
err_out_lock:
    // successful unlock will reset err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "could not unlock task manager. Possible deadlock!\n");
    err = err2;
err_out:
    return (err);
}

struct benesh_cohort *benesh_get_cohort(struct benesh_handle *bnh)
{
    TRACE_OUT;
    if(!bnh) {
        return (NULL);
    }
    return (bnh->bco);
}

struct benesh_rule *benesh_get_rule_by_id(struct benesh_handle *bnh, int id)
{
    TRACE_OUT;
    struct benesh_rule *rule;

    if(!bnh) {
        return (NULL);
    }

    rule = benesh_rule_get_by_id(bnh->rules, id);
    return (rule);
}

struct benesh_component *benesh_get_comp_by_id(struct benesh_handle *bnh,
                                               int id)
{
    TRACE_OUT;
    struct benesh_component *comp;

    if(!bnh) {
        return (NULL);
    }

    comp = benesh_comp_get_by_id(bnh->bco, id);
    return (comp);
}

struct benesh_touchpoint *benesh_get_tpoint_by_id(struct benesh_handle *bnh,
                                                  int id)
{
    TRACE_OUT;
    struct benesh_touchpoint *btp;

    if(!bnh) {
        return (NULL);
    }

    btp = benesh_tp_get_by_id(bnh->tpoints, id);
    return (btp);
}

int benesh_add_import_task_by_ids(struct benesh_handle *bnh, int rule_id,
                                  int subrule_id, int64_t *tgt_vars)
{
    TRACE_OUT;
    struct benesh_work_node *wnode;
    struct benesh_rule *rule;
    size_t nvar;
    int i, err;

    if(!bnh || !bnh->btm || !bnh->rules) {
        ERR_OUT(BNH_EINVAL, err_out, "bad benesh handle.\n");
    }

    if(rule_id < 0 || rule_id >= benesh_get_num_rules(bnh->rules)) {
        ERR_OUT(BNH_EINVAL, err_out, "bad rule id %i.", rule_id);
    }

    ASSIGN_NOT_NULL(benesh_get_rule_by_id(bnh, rule_id), rule, err, err_out,
                    "could get access rule.\n");
    if(subrule_id < 0 || subrule_id >= benesh_get_num_subrules(rule)) {
        ERR_OUT(BNH_EINVAL, err_out, "bad subrule id %i.\n", subrule_id);
    }

    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), err, err_out,
               "could not access rule.\n");
    if(nvar && !tgt_vars) {
        ERR_OUT(BNH_EFAULT, err_out, "missing mapped values.\n");
    }

    CHECK_ZERO(benesh_enqueue_import(bnh->btm, rule, subrule_id, tgt_vars), err,
               err_out, "failed to enqueue import task.\n");

    return (0);

err_out:
    return (err);
}

void benesh_sleep_til_ready(struct benesh_handle *bnh)
{
    TRACE_OUT;
    int time_asleep = 0;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    while(!bnh->f_ready) {
        // Change to wait condition? Only toggles once.
        sleep(1);
        time_asleep++;
        if(time_asleep % 10 == 0) {
            DEBUG_OUT("I have slept %i seconds waiting for the workflow to be "
                      "ready.\n",
                      time_asleep);
        }
    }
err_out:
    return;
}

int benesh_my_comp_id(struct benesh_handle *bnh, int *comp_id)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    struct bnh_pvec *cvec;
    int err, flag;

    if(!bnh || !bnh->bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    cvec = benesh_get_components(bnh->bco);
    BNH_PVEC_FOREACH(comp, bi, cvec)
    {
        CHECK_ZERO(benesh_comp_is_me(comp, &flag), BNH_EFAULT, err_out,
                   "component inspection failed.\n");
        if(flag) {
            CHECK_ZERO(benesh_comp_id(comp, comp_id), err, err_out,
                       "failed to retrieve component ID.\n");
            return (0);
        }
    }

    *comp_id = BNH_COMP_NULL;
    ERR_OUT(BNH_ESTATE, err_out, "could not find my own component.\n")
err_out:
    return (err);
}

int benesh_disconnect(struct benesh_handle *bnh, int comp_id, int *remaining)
{
    TRACE_OUT;
    int err, err2;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(comp_id < 0 || comp_id >= benesh_get_component_count(bnh->bco)) {
        ERR_OUT(BNH_EINVAL, err_out, "bad component id %i\n", comp_id);
    }

    /*
        A component disconnecting might progress (or terminate) the task
       manager. Signal the task manager when a disconnect occurs.
    */
    CHECK_ZERO(benesh_taskman_lock(bnh->btm), err, err_out,
               "failed to lock task manager.\n");
    CHECK_ZERO(benesh_comp_disconnect(bnh->bco, comp_id), err, err_out_signal,
               "failed to disconnect component %" PRIu32 "\n", comp_id);
    if(remaining) {
        benesh_connected_count(bnh->bco, remaining);
    }
    CHECK_ZERO(benesh_taskman_signal(bnh->btm), err, err_out_unlock,
               "failed to signal task manager.\n");
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");

err_out_signal:
    // successful signal will clear err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_taskman_signal(bnh->btm), err, err_out_unlock,
               "failed to signal task manager.\n");
    err = err2;
err_out_unlock:
    err2 = err;
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");
    err = err2;
err_out:
    return (err);
}

int benesh_schedule_target(struct benesh_handle *bnh, struct benesh_obj *target)
{
    TRACE_OUT;
    struct benesh_rule *rule;
    int64_t *var_map;
    int err, err2;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!target) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }

    CHECK_ZERO(benesh_rule_match_target(bnh->rules, &rule, &var_map),
               BNH_ENOENT, err_out, "cannot match target to workflow rule.\n");
    CHECK_ZERO(benesh_taskman_lock(bnh->btm), err, err_out,
               "could not lock task manager.\n");
    CHECK_ZERO(benesh_taskman_schedule_rule(bnh, bnh->btm, rule, var_map), err,
               err_out_lock, "could not schedule rule.\n");
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "could not unlock task manager. Possible deadlock!\n");

    return (0);
err_out_lock:
    // succesful unlock will reset err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "could not unlock task manager. Possible deadlock!\n");
    err = err2;
err_out:
    return (err);
}