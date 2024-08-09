#include "benesh-ekt.h"
#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-targets.h"
#include "benesh-tasks.h"
#include "benesh-timing.h"
#include "benesh-tpoint.h"
#include "benesh-types.h"
#include "benesh.h"
#include "beneshp.h"
#include "util.h"
#include <ekt.h>

#include <inttypes.h>
#include <unistd.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* temp */
struct obj_entry *get_object_entry(struct benesh_handle *bnh,
                                   struct wf_target *rule, int subrule_id,
                                   int64_t *map_vals, int create);

int activate_subs(struct benesh_handle *bnh, struct work_node *wnode);
char *tpoint_tostr(const char *comp_name, struct tpoint_rule *rule);
struct pq_obj *resolve_obj(struct benesh_handle *bnh, struct xc_list_node *obj,
                           int nmappings, char **map_names, int64_t *vals);
int schedule_target(struct benesh_handle *bnh, struct pq_obj *tgt);
/* temp */

static int serialize_work(void *work_v, void *bnh_v, void **buf)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = (struct work_announce *)work_v;
    struct benesh_rule *rule;
    size_t buf_size;
    size_t nmap;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer.\n");
    }

    if(!work) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.");
    }

    ASSIGN_NOT_NULL(benesh_get_rule_by_id(bnh, work->rule_id), rule, BNH_ESYNC,
                    err_out, "received work that does not match a rule.\n");
    CHECK_ZERO(benesh_rule_get_nvar(rule, &nmap), BNH_EFAULT, err_out,
               "target access failed.\n");

    buf_size = sizeof(work->comp_id) + sizeof(work->rule_id) +
               sizeof(work->subrule_id) + nmap * sizeof(*work->tgt_vars);
    ASSIGN_NOT_NULL(malloc(buf_size), *buf, BNH_ENOMEM, err_out,
                    "could not allocate buffer.\n");

    ((uint32_t *)(*buf))[0] = work->comp_id;
    ((uint32_t *)(*buf))[1] = work->rule_id;
    ((uint32_t *)(*buf))[2] = work->subrule_id;
    if(nmap) {
        memcpy(&((uint32_t *)(*buf))[3], work->tgt_vars,
               nmap * sizeof(*work->tgt_vars));
    }

    return (buf_size);
err_out:
    return (-1);
}

static int deserialize_work(void *buf, void *bnh_v, void **work_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work;
    struct benesh_rule *rule;
    size_t nmap;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer.\n");
    }

    if(!work_v) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target structure.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*work)), work, BNH_ENOMEM, err_out,
                    "could not allocate buffer.\n");

    work->comp_id = ((uint32_t *)buf)[0];
    work->rule_id = ((uint32_t *)buf)[1];
    work->subrule_id = ((uint32_t *)buf)[2];
    work->tgt_vars = NULL;
    ASSIGN_NOT_NULL(benesh_get_rule_by_id(bnh, work->rule_id), rule, BNH_ESYNC,
                    err_out, "received work that does not match a rule.\n");
    CHECK_ZERO(benesh_rule_get_nvar(rule, &nmap), BNH_EFAULT, err_out,
               "target access failed.\n");
    if(nmap) {
        ASSIGN_NOT_NULL(malloc(nmap * sizeof(*work->tgt_vars)), work->tgt_vars,
                        BNH_ENOMEM, err_out,
                        "could not allocate target variables.\n");
        memcpy(work->tgt_vars, &((uint32_t *)buf)[3],
               nmap * sizeof(*work->tgt_vars));
    }

    *work_v = work;

    return (0);
err_out:
    return (err);
}

static int work_watch(void *work_v, void *bnh_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = (struct work_announce *)work_v;
    int my_comp_id;
    size_t nvar;
    int i, err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!work) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.\n");
    }

    benesh_sleep_til_ready(bnh);

    DEBUG_OUT("received work from comp %" PRIu32 ", tgt_id = %" PRIu32
              ", subrule = %" PRIu32 "\n",
              work->comp_id, work->rule_id, work->subrule_id);

    CHECK_ZERO(benesh_my_comp_id(bnh, &my_comp_id), err, err_out,
               "could not retrieve my own component ID.\n");
    if(work->comp_id == my_comp_id) {
        DEBUG_OUT("work is from myself...ignoring.\n");
        return (0);
    }

    benesh_add_import_task_by_ids(bnh, work->rule_id, work->subrule_id,
                                  work->tgt_vars);

    return (0);
err_out:
    return (err);
}

static int serialize_tpoint(void *tpoint_v, void *bnh_v, void **buf)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_rule *rules;
    struct tpoint_announce *tpoint = (struct tpoint_announce *)tpoint_v;
    struct benesh_touchpoint *btp;
    size_t nmap;
    size_t buf_size;
    int64_t *mappings;
    int i, err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer.\n");
    }

    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.");
    }

    ASSIGN_NOT_NULL(benesh_get_tpoint_by_id(bnh, tpoint->rule_id), btp,
                    BNH_ESYNC, err_out,
                    "received work that does not match a rule.\n");
    CHECK_ZERO(benesh_tp_get_nvar(btp, &nmap), BNH_EFAULT, err_out,
               "target access failed.\n");

    buf_size =
        sizeof(tpoint->rule_id) + tpoint->comp_id + (nmap * sizeof(*mappings));
    *buf = malloc(buf_size);
    ASSIGN_NOT_NULL(malloc(buf_size), *buf, BNH_ENOMEM, err_out,
                    "could not allocate buffer.\n");

    ((uint32_t *)(*buf))[0] = tpoint->rule_id;
    ((uint32_t *)(*buf))[1] = tpoint->comp_id;
    mappings = (int64_t *)((uint64_t)(*buf) + sizeof(tpoint->rule_id) +
                           sizeof(tpoint->comp_id));
    if(nmap) {
        memcpy(mappings, tpoint->tp_vars, nmap * sizeof(*tpoint->tp_vars));
    }

    return (buf_size);
err_out:
    return (-1);
}

static int deserialize_tpoint(void *buf, void *bnh_v, void **tpoint_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_announce *tpoint;
    struct benesh_touchpoint *btp;
    size_t nmap;
    int64_t *mappings;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer.\n");
    }

    if(!tpoint_v) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target structure.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*tpoint)), tpoint, BNH_ENOMEM, err_out,
                    "could not allocate buffer.\n");

    tpoint->rule_id = ((uint32_t *)buf)[0];
    tpoint->comp_id = ((uint32_t *)buf)[1];
    ASSIGN_NOT_NULL(benesh_get_tpoint_by_id(bnh, tpoint->rule_id), btp,
                    BNH_ESYNC, err_out,
                    "received work that does not match a rule.\n");
    CHECK_ZERO(benesh_tp_get_nvar(btp, &nmap), BNH_EFAULT, err_out,
               "target access failed.\n");

    mappings = (int64_t *)((uint64_t)buf + sizeof(tpoint->rule_id) +
                           sizeof(tpoint->comp_id));
    tpoint->tp_vars = malloc(sizeof(*tpoint->tp_vars) * nmap);
    memcpy(tpoint->tp_vars, mappings, nmap * sizeof(*tpoint->tp_vars));

    *tpoint_v = tpoint;

    return (0);
err_out:
    return (err);
}

static int tpoint_watch(void *tpoint_v, void *bnh_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_rule *rules;
    struct tpoint_announce *tpoint = (struct tpoint_announce *)tpoint_v;
    struct benesh_component *comp, *tp_comp;
    struct benesh_touchpoint *btp;
    struct pq_obj **fq_tgts;
    struct xc_list_node *tgt_obj;
    struct tpoint_rule *rule;
    char *comp_name, *tp_comp_name;
    ;
    int i, err;

    comp_name = tp_comp_name = NULL;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.\n");
    }

    benesh_sleep_til_ready(bnh);

    ASSIGN_NOT_NULL(benesh_get_tpoint_by_id(bnh, tpoint->rule_id), btp,
                    BNH_ESYNC, err_out, "receive unknown touchpoint id.\n");
    ASSIGN_NOT_NULL(benesh_get_comp_by_id(bnh, tpoint->comp_id), comp,
                    BNH_ESYNC, err_out, "received unknown component id.\n");
    ASSIGN_NOT_NULL(benesh_tp_get_comp(btp), tp_comp, BNH_ESTATE, err_out,
                    "could not access touchpoint info.\n");
    if(comp != tp_comp) {
        ASSIGN_NOT_NULL(benesh_comp_name(comp), comp_name, BNH_ESTATE, err_out,
                        "could not access component info.\n");
        ASSIGN_NOT_NULL(benesh_comp_name(tp_comp), tp_comp_name, BNH_ESTATE,
                        err_out, "could not access component info.\n");
        WARN_OUT("Received touchpoint id %" PRIu32
                 "from non-matching component %s. Expected %s.\n",
                 tpoint->rule_id, comp_name, tp_comp_name);
        free(comp_name);
        free(tp_comp_name);
        comp_name = tp_comp_name = NULL;
    }

    /*
        if(benesh_debug_enabled()) {
            ASSIGN_NOT_NULL(benesh_tp_to_str(btp, tpoint->tp_vars), tp_str,
       BNH_ESTATE, err_out, "could not stringify announced touchpoint.");
            DEBUG_OUT("Received touchpoint %s from component %s.\n", tp_str,
       comp_name); free(tp_str);
        }

        DEBUG_OUT("rule has %i targets\n", rule->num_tgts);
        fq_tgts = malloc(sizeof(*fq_tgts) * rule->num_tgts);
        for(i = 0; i < rule->num_tgts; i++) {
            tgt_obj = rule->tgts[i];
            fq_tgts[i] = resolve_obj(bnh, tgt_obj, rule->nmappings,
       rule->map_names, tpoint->tp_vars);
            // This is a rather large critical section, and it blocks progress
            // handling.
            APEX_NAME_TIMER_START(1, "work_lock_twa");
            ABT_mutex_lock(bnh->work_mutex);
            APEX_TIMER_STOP(1);
            schedule_target(bnh, fq_tgts[i]);
            ABT_cond_signal(bnh->work_cond);
            ABT_mutex_unlock(bnh->work_mutex);
        }
    */
    return (0);
err_out:
    if(comp_name)
        free(comp_name);
    if(tp_comp_name)
        free(tp_comp_name);
    return (err);
}

static int serialize_fini(void *fini_v, void *bnh_v, void **buf)
{
    TRACE_OUT;
    uint32_t *comp_id = (uint32_t *)fini_v;
    int err;

    if(!*buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer pointer.\n");
    }
    if(!comp_id) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.\n");
    }
    ASSIGN_NOT_NULL(malloc(sizeof(*comp_id)), *buf, BNH_ENOMEM, err_out,
                    "could not allocate buffer.\n");
    *(uint32_t *)(*buf) = *comp_id;

    return (sizeof(*comp_id));
err_out:
    return (-1);
}

static int deserialize_fini(void *buf, void *bnh_v, void **fini_v)
{
    TRACE_OUT;
    uint64_t *comp_id;
    int err;

    if(!buf) {
        ERR_OUT(BNH_EFAULT, err_out, "bad buffer.\n");
    }

    if(!fini_v) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target structure.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*comp_id)), comp_id, BNH_ENOMEM, err_out,
                    "could not allocate component ID.\n");
    *comp_id = *(uint32_t *)buf;

    *fini_v = comp_id;

    return (0);
err_out:
    return (err);
}

static int fini_watch(void *fini_v, void *bnh_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    int comp_id, conn_count;
    int err, err2;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    if(!fini_v) {
        ERR_OUT(BNH_EFAULT, err_out, "bad input structure.\n");
    }

    comp_id = *(uint32_t *)fini_v;

    CHECK_ZERO(benesh_disconnect(bnh, comp_id, &conn_count), err, err_out,
               "failed to disconnect component %i\n", comp_id);
    if(benesh_debug_enabled()) {
        DEBUG_OUT("Handled finalize from component %i. %i remaining\n", comp_id,
                  conn_count);
    }

    return (0);
err_out:
    return (err);
}

struct bnhekt_handle *benesh_ekt_init(const char *name, MPI_Comm comm,
                                      margo_instance_id mid, void *bnhv)
{
    TRACE_OUT;
    struct bnhekt_handle *bekth;
    int err;

    if(!name || !*name) {
        ERR_OUT(BNH_EINVAL, err_out, "name missing.\n");
    }

    bekth = malloc(sizeof(*bekth));

    DEBUG_OUT("initializing EKT.\n");

    CHECK_ZERO(ekt_init(&bekth->ekth, name, comm, mid), BNH_EEKT, err_out,
               "ekt_init failed with %i\n", err);

    // CHECK_ZERO(ekt_register(bekth->ekth, BNH_EKT_WORK, serialize_work,
    // deserialize_work, bnhv, &bekth->work_type), BNH_EEKT, err_out,
    // "ekt_register failed with %i\n", err); CHECK_ZERO(ekt_watch(bekth->ekth,
    // bekth->work_type, work_watch), BNH_EEKT, err_out, "ekt_watch failed with
    // %i\n", err);

    CHECK_ZERO(ekt_register(bekth->ekth, BNH_EKT_FINI, serialize_fini,
                            deserialize_fini, bnhv, &bekth->fini_type),
               BNH_EEKT, err_out, "ekt_register failed with %i\n", err);
    CHECK_ZERO(ekt_watch(bekth->ekth, bekth->fini_type, fini_watch), BNH_EEKT,
               err_out, "ekt_watch failed with %i\n", err);

    // CHECK_ZERO(ekt_register(bekth->ekth, BNH_EKT_TP, serialize_tpoint,
    // deserialize_tpoint, bnhv, &bekth->tp_type), BNH_EEKT, err_out,
    // "ekt_register failed with %i\n", err); CHECK_ZERO(ekt_watch(bekth->ekth,
    // bekth->tp_type, tpoint_watch), BNH_EEKT, err_out, "ekt_watch failed with
    // %i\n", err);

    DEBUG_OUT("EKT initialized.\n");

    return (bekth);

err_out:
    return (NULL);
}

int benesh_ekt_send_fini(struct bnhekt_handle *bekth, uint32_t comp_id)
{
    TRACE_OUT;
    int err;

    if(!bekth) {
        ERR_OUT(BNH_EINVAL, err_out, "bad handle.\n");
    }

    DEBUG_OUT("sending fini\n");
    CHECK_ZERO(ekt_tell(bekth->ekth, NULL, bekth->fini_type, &comp_id),
               BNH_EEKT, err_out, "ekt_tell failed.\n");

    return (0);
err_out:
    return (err);
}

int benesh_ekt_xconnect(struct bnhekt_handle *bekth, struct benesh_cohort *bco,
                        MPI_Comm comm, int wait)
{
    TRACE_OUT;
    int rank;
    struct benesh_component *comp;
    char *name;
    struct bnh_pvec *cvec;
    bnh_pvec_iter bi;
    int i, err, flag;

    if(!bekth) {
        ERR_OUT(BNH_EFAULT, err_out, "bad handle.\n");
    }

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    ekt_enable(bekth->ekth);
    if(wait) {
        CHECK_ZERO(MPI_Comm_rank(comm, &rank), BNH_EINVAL, err_out,
                   "bad communicator.\n");
        if(!rank) {
            DEBUG_OUT("waiting for bidirectional communication with other "
                      "components.\n");
            ASSIGN_NOT_NULL(benesh_get_components(bco), cvec, BNH_EFAULT,
                            err_out, "failed to get components.");
            BNH_PVEC_FOREACH(comp, bi, cvec)
            {
                CHECK_ZERO(benesh_comp_is_me(comp, &flag), BNH_EFAULT, err_out,
                           "bad component pointer.\n");
                if(!flag) {
                    ASSIGN_NOT_NULL(benesh_comp_name(comp), name, BNH_EFAULT,
                                    err_out, "failed to get component name.\n");
                    DEBUG_OUT("achieving bidi status with component '%s'\n",
                              name);
                    ekt_is_bidi(bekth->ekth, name, 1);
                }
            }
        }
        CHECK_ZERO(MPI_Barrier(comm), BNH_EINVAL, err_out,
                   "invalid communicator.\n");
    } else {
        DEBUG_OUT("proceeding without waiting for other components.\n");
    }

    return (0);

err_out:
    return (err);
}

#if defined(__cplusplus)
}
#endif