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
                                  int directive_id, int64_t *var_map)
{
    TRACE_OUT;
    struct benesh_work_node *wnode;
    struct benesh_rule *rule;
    struct benesh_target *tgt;
    size_t nvar, ndir;
    int i, err, err2;

    if(!bnh || !bnh->btm || !bnh->rules || !bnh->bdb) {
        ERR_OUT(BNH_EINVAL, err_out, "bad benesh handle.\n");
    }

    if(rule_id < 0 || rule_id >= benesh_get_num_rules(bnh->rules)) {
        ERR_OUT(BNH_EINVAL, err_out, "bad rule id %i.", rule_id);
    }

    ASSIGN_NOT_NULL(benesh_get_rule_by_id(bnh, rule_id), rule, err, err_out,
                    "could get access rule.\n");
    ASSIGN_NOT_NULL(benesh_tgt_db_lookup(bnh->bdb, rule, var_map), tgt,
                    BNH_ENOENT, err_out, "could not lookup target.\n");
    CHECK_ZERO(benesh_rule_get_ndir(rule, &ndir), err, err_out,
               "could not access rule.\n");
    if(directive_id < 0 || directive_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "bad directive id %i.\n", directive_id);
    }

    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), err, err_out,
               "could not access rule.\n");
    if(nvar && !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "missing mapped values.\n");
    }

    CHECK_ZERO(benesh_taskman_lock(bnh->btm), err, err_out,
               "failed to lock task manager.\n");

    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "failed to unlock task manager. Probable deadlock!\n");
    CHECK_ZERO(benesh_enqueue_import(bnh->btm, tgt, directive_id), err, err_out,
               "failed to enqueue import task.\n");

    return (0);

err_out_lock:
    // successful unlock clears err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_taskman_unlock(bnh->btm), err, err_out,
               "failed to unlock task manager. Probable deadlock!\n");
    err = err2;
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

struct benesh_component *benesh_my_comp(struct benesh_handle *bnh)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    struct bnh_pvec *cvec;
    int err, flag;

    if(!bnh || !bnh->bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    ASSIGN_NOT_NULL(benesh_get_components(bnh->bco), cvec, BNH_ESTATE, err_out,
                    "could not access cohort handle.\n");
    BNH_PVEC_FOREACH(comp, bi, cvec)
    {
        CHECK_ZERO(benesh_comp_is_me(comp, &flag), BNH_EFAULT, err_out,
                   "component inspection failed.\n");
        if(flag) {
            return (comp);
        }
    }

    ERR_OUT(BNH_ESTATE, err_out, "could not find my own component.\n")
err_out:
    return (NULL);
}

int benesh_my_comp_id(struct benesh_handle *bnh, int *comp_id)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    struct bnh_pvec *cvec;
    int err, flag;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!comp_id) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    ASSIGN_NOT_NULL(benesh_my_comp(bnh), comp, BNH_ESTATE, err_out,
                    "could not find my own component.\n");
    CHECK_ZERO(benesh_comp_id(comp, comp_id), err, err_out,
               "cou[le not access component.\n");

    return (0);
err_out:
    *comp_id = BNH_COMP_NULL;
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

int benesh_obj_to_tpoint(struct benesh_handle *bnh, struct benesh_obj *obj,
                         struct benesh_touchpoint **tpoint, int64_t **var_map)
{
    TRACE_OUT;
    struct benesh_component *my_comp;
    int is_resolved;
    int err;

    if(!bnh || !bnh->bco || !bnh->tpoints) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    if(!tpoint || !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target pointer.\n");
    }

    *var_map = NULL;

    CHECK_ZERO(benesh_obj_fully_resolved(obj, &is_resolved), BNH_EFAULT,
               err_out, "could not check object.\n");
    if(!is_resolved) {
        ERR_OUT(BNH_EINVAL, err_out, "object not fully resolved.\n");
    }
    ASSIGN_NOT_NULL(benesh_my_comp(bnh), my_comp, BNH_EFAULT, err_out,
                    "could not find my own component.\n");
    CHECK_ZERO(
        benesh_tp_find_viable(bnh->tpoints, my_comp, obj, tpoint, var_map),
        BNH_ENOENT, err_out,
        "could not find viable rule to match prerequesite target.\n");

    return (0);
err_out:
    if(*tpoint)
        *tpoint = NULL;
    if(*var_map) {
        free(*var_map);
        *var_map = NULL;
    }

    return (err);
}

struct benesh_target *benesh_obj_to_tgt(struct benesh_handle *bnh,
                                        struct benesh_obj *obj)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    struct benesh_rule *rule;
    int64_t *var_map = NULL;
    int is_resolved;
    size_t nvar;
    int err;

    if(!bnh || !bnh->rules || !bnh->bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    CHECK_ZERO(benesh_obj_fully_resolved(obj, &is_resolved), BNH_EFAULT,
               err_out, "could not check object.\n");
    if(!is_resolved) {
        ERR_OUT(BNH_EINVAL, err_out, "object not fully resolved.\n");
    }
    CHECK_ZERO(benesh_rule_find_viable(bnh->rules, obj, &rule, &var_map),
               BNH_ENOENT, err_out,
               "could not find viable rule to match object.\n");
    ASSIGN_NOT_NULL(benesh_tgt_db_lookup(bnh->bdb, rule, var_map), tgt,
                    BNH_ENOENT, err_out,
                    "could not locate target in database.\n");
    free(var_map);
    return (tgt);
err_out:
    if(var_map)
        free(var_map);
    return (NULL);
}

struct benesh_target *benesh_obj_resolve_to_tgt(struct benesh_handle *bnh,
                                                struct benesh_obj *obj,
                                                int64_t *var_map)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    struct benesh_obj *resolved_obj;
    struct benesh_rule *rule;
    size_t nvar;
    int err;

    if(!bnh || !bnh->rules || !bnh->bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    CHECK_ZERO(benesh_obj_nvars(obj, &nvar), err, err_out,
               "could not access object.\n");
    if(nvar && !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "variable map should not be empty.\n");
    }
    ASSIGN_NOT_NULL(benesh_obj_resolve(obj, var_map), resolved_obj, BNH_ESTATE,
                    err_out, "resolution of object failed.\n");
    ASSIGN_NOT_NULL(benesh_obj_to_tgt(bnh, resolved_obj), tgt, BNH_ENOENT,
                    err_out, "could not find viable target to match object.\n");

    return (tgt);
err_out:
    return (NULL);
}

int benesh_schedule_obj(struct benesh_handle *bnh, struct benesh_obj *obj)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    int err, err2;

    if(!bnh || !bnh->btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    CHECK_ZERO(benesh_taskman_lock(bnh->btm), err, err_out,
               "could not lock task manager.\n");
    ASSIGN_NOT_NULL(benesh_obj_to_tgt(bnh, obj), tgt, BNH_ENOENT, err_out_lock,
                    "could not find matching target for object.\n");
    CHECK_ZERO(benesh_taskman_schedule_target(bnh, tgt), err, err_out_lock,
               "could not schedule rule.\n");
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

struct benesh_target *benesh_target_lookup(struct benesh_handle *bnh,
                                           struct benesh_rule *rule,
                                           int64_t *var_map)
{
    TRACE_OUT;
    size_t nvar;
    struct benesh_target *tgt;
    int err;

    if(!bnh || !bnh->bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), err, err_out,
               "could not access rule.\n");
    if(nvar && !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "empty variable map.\n");
    }

    ASSIGN_NOT_NULL(benesh_tgt_db_lookup(bnh->bdb, rule, var_map), tgt,
                    BNH_ESTATE, err_out, "could not lookup target.\n");

    return (tgt);
err_out:
    return (NULL);
}

int benesh_get_target_status(struct benesh_handle *bnh,
                             struct benesh_rule *rule, int64_t *var_map,
                             int *status)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    if(!status) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    ASSIGN_NOT_NULL(benesh_tgt_db_lookup(bnh->bdb, rule, var_map), tgt,
                    BNH_ENOENT, err_out,
                    "could not locate target in database.\n");
    CHECK_ZERO(benesh_target_get_status(tgt, status), err, err_out,
               "could not access target.\n");

    return (0);
err_out:
    return (err);
}