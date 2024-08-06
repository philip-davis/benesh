#include "benesh-ekt.h"
#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-tasks.h"
#include "benesh-timing.h"
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

/*
int deserialize_work(void *buf, void *bnh_v, void **work_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = malloc(sizeof(*work));
    struct wf_target *tgt;
    size_t nmap;

    assert(0 && "not implemented");

    work->comp_id = ((uint32_t *)buf)[0];
    work->tgt_id = ((uint32_t *)buf)[1];
    work->subrule_id = ((uint32_t *)buf)[2];
    tgt = &bnh->tgts[work->tgt_id];
    nmap = tgt->num_vars;
    work->tgt_vars = malloc(nmap * sizeof(*work->tgt_vars));
    memcpy(work->tgt_vars, &((uint32_t *)buf)[3],
           nmap * sizeof(*work->tgt_vars));

    *work_v = work;

    return (0);
}

int serialize_work(void *work_v, void *bnh_v, void **buf)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = (struct work_announce *)work_v;
    struct wf_target *tgt = &bnh->tgts[work->tgt_id];
    size_t buf_size;
    size_t nmap = tgt->num_vars;

    buf_size = sizeof(work->comp_id) + sizeof(work->tgt_id) +
               sizeof(work->subrule_id) + nmap * sizeof(*work->tgt_vars);
    *buf = malloc(buf_size);

    ((uint32_t *)(*buf))[0] = work->comp_id;
    ((uint32_t *)(*buf))[1] = work->tgt_id;
    ((uint32_t *)(*buf))[2] = work->subrule_id;
    memcpy(&((uint32_t *)(*buf))[3], work->tgt_vars,
           nmap * sizeof(*work->tgt_vars));

    return (buf_size);
}

int work_watch(void *work_v, void *bnh_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = (struct work_announce *)work_v;
    struct work_node wnode;
    struct obj_entry *ent;
    int i;

    APEX_FUNC_TIMER_START(work_watch);
    while(!bnh->ready) {
        // Change to wait condition
        sleep(1);
    }

    DEBUG_OUT("received work from comp %" PRIu32 ", tgt_id = %" PRIu32
              ", subrule = %" PRIu32 "\n",
              work->comp_id, work->tgt_id, work->subrule_id);

    if(work->comp_id == bnh->comp_id) {
        DEBUG_OUT("work is from myself...ignoring.\n");
        APEX_TIMER_STOP(0);
        return 0;
    }

    wnode.type = BNH_WORK_ANNOUNCE;
    wnode.tgt = &bnh->tgts[work->tgt_id];
    wnode.subrule = work->subrule_id;
    wnode.var_maps = malloc(sizeof(*wnode.var_maps) * wnode.tgt->num_vars);
    memcpy(wnode.var_maps, work->tgt_vars,
           sizeof(*wnode.var_maps) * wnode.tgt->num_vars);

    DEBUG_OUT("received work for target %" PRIu32 ", subrule %" PRId32 "\n",
              work->tgt_id, work->subrule_id);
    if(bnh->f_debug) {
        for(i = 0; i < wnode.tgt->num_vars; i++) {
            DEBUG_OUT(" tgt_var %i = %" PRIu64 "\n", i, wnode.var_maps[i]);
        }
    }
    APEX_NAME_TIMER_START(1, "db_lock_wwa");
    ABT_mutex_lock(bnh->db_mutex);
    APEX_TIMER_STOP(1);
    ent = get_object_entry(bnh, wnode.tgt, wnode.subrule, wnode.var_maps, 1);
    if(!ent) {
        fprintf(stderr,
                "ERROR: null entry when realizing work (shouldn't happen).\n");
    }
    if(ent->realized) {
        fprintf(stderr,
                "WARNING: trying to realize work that already is realized:\n");
    }
    ent->realized = 1;
    DEBUG_OUT(" realized entry %p\n", (void *)ent);

    ABT_mutex_unlock(bnh->db_mutex);
    activate_subs(bnh, &wnode);
    // activate_subs only signals the handler if some sub is satsified. This
object being
    // realized may mean we are done with a touchpoint.
    DEBUG_OUT("Signalling handler to restart\n");
    ABT_cond_signal(bnh->work_cond);
    APEX_TIMER_STOP(0);

    return (0);
}

int serialize_tpoint(void *tpoint_v, void *bnh_v, void **buf)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_rule *rules = bnh->tph->rules;
    struct tpoint_announce *tpoint = (struct tpoint_announce *)tpoint_v;
    size_t nmappings = rules[tpoint->rule_id].nmappings;
    size_t buf_size;
    uint32_t rule_id = tpoint->rule_id;
    uint32_t comp_id = tpoint->comp_id;
    int64_t *mappings;
    int i;

    buf_size =
        sizeof(rule_id) + sizeof(comp_id) + (nmappings * sizeof(*mappings));
    *buf = malloc(buf_size);

    ((uint32_t *)(*buf))[0] = rule_id;
    ((uint32_t *)(*buf))[1] = comp_id;
    mappings = *buf + 2 * sizeof(uint32_t);
    for(i = 0; i < nmappings; i++) {
        mappings[i] = tpoint->tp_vars[i];
    }

    return (buf_size);
}


int deserialize_tpoint(void *buf, void *bnh_v, void **tpoint_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_rule *rules = bnh->tph->rules;
    struct tpoint_announce *tpoint = malloc(sizeof(*tpoint));
    uint32_t rule_id = ((uint32_t *)buf)[0];
    uint32_t comp_id = ((uint32_t *)buf)[1];
    size_t nmappings = rules[rule_id].nmappings;
    int64_t *mappings;
    int i;

    tpoint->rule_id = rule_id;
    tpoint->comp_id = comp_id;
    mappings = buf + 2 * sizeof(uint32_t);
    tpoint->tp_vars = malloc(sizeof(*tpoint->tp_vars) * nmappings);
    for(i = 0; i < nmappings; i++) {
        tpoint->tp_vars[i] = mappings[i];
    }

    *tpoint_v = tpoint;

    return (0);
}

int tpoint_watch(void *tpoint_v, void *bnh_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct tpoint_rule *rules = bnh->tph->rules;
    struct tpoint_announce *tpoint = (struct tpoint_announce *)tpoint_v;
    struct pq_obj **fq_tgts;
    struct xc_list_node *tgt_obj;
    uint32_t rule_id = tpoint->rule_id;
    struct tpoint_rule *rule = &rules[rule_id];
    int i;

    APEX_FUNC_TIMER_START(tpoint_watch);
    while(!bnh->ready) {
        // Change to wait condition
        sleep(1);
    }

    if(bnh->f_debug) {
        DEBUG_OUT("Touchpoint announcement received for rule %s\n",
                tpoint_tostr(bnh->comps[tpoint->comp_id].name, rule));
        DEBUG_OUT("  with the mappings:\n");
        for(i = 0; i < rule->nmappings; i++) {
            DEBUG_OUT("     [%s] => %" PRId64 "\n", rule->map_names[i],
tpoint->tp_vars[i]);
        }
    }

    DEBUG_OUT("rule has %i targets\n", rule->num_tgts);
    fq_tgts = malloc(sizeof(*fq_tgts) * rule->num_tgts);
    for(i = 0; i < rule->num_tgts; i++) {
        tgt_obj = rule->tgts[i];
        fq_tgts[i] = resolve_obj(bnh, tgt_obj, rule->nmappings, rule->map_names,
                                 tpoint->tp_vars);
        // This is a rather large critical section, and it blocks progress
        // handling.
        APEX_NAME_TIMER_START(1, "work_lock_twa");
        ABT_mutex_lock(bnh->work_mutex);
        APEX_TIMER_STOP(1);
        schedule_target(bnh, fq_tgts[i]);
        ABT_cond_signal(bnh->work_cond);
        ABT_mutex_unlock(bnh->work_mutex);
    }
    APEX_TIMER_STOP(0);
    return 0;
}
*/
static int serialize_fini(void *fini_v, void *bnh_v, void **buf)
{
    TRACE_OUT;
    uint32_t *comp_id = (uint32_t *)fini_v;

    *buf = malloc(sizeof(*comp_id));
    *(uint32_t *)(*buf) = *comp_id;

    return (sizeof(*comp_id));
}

static int deserialize_fini(void *buf, void *bnh_v, void **fini_v)
{
    TRACE_OUT;
    uint64_t *comp_id = malloc(sizeof(*comp_id));

    *comp_id = *(uint32_t *)buf;

    *fini_v = comp_id;

    return 0;
}

static int fini_watch(void *fini_v, void *bnh_v)
{
    TRACE_OUT;
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct benesh_taskman *btm;
    struct benesh_cohort *bco;
    uint32_t comp_id = *(uint32_t *)fini_v;
    int conn_count;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }

    bco = benesh_get_cohort(bnh);
    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort handle.\n");
    }

    btm = benesh_get_taskman(bnh);
    if(!btm) {
        ERR_OUT(BNH_EFAULT, err_out, "bad task manager handle.\n");
    }

    CHECK_ZERO(benesh_taskman_lock(btm), err, err_out,
               "failed to lock task manager.\n");
    CHECK_ZERO(benesh_disconnect(bco, comp_id), err, err_out_signal,
               "failed to disconnect component %" PRIu32 "\n", comp_id);
    if(benesh_debug_enabled()) {
        benesh_connected_count(bco, &conn_count);
        DEBUG_OUT("Got finalize from component %" PRIu32 ". %i remaining\n",
                  comp_id, conn_count);
    }

    CHECK_ZERO(benesh_taskman_signal(btm), err, err_out_unlock,
               "failed to signal task manager.\n");
    CHECK_ZERO(benesh_taskman_unlock(btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");

    return (0);

err_out_signal:
    CHECK_ZERO(benesh_taskman_signal(btm), err, err_out_unlock,
               "failed to signal task manager.\n");
err_out_unlock:
    CHECK_ZERO(benesh_taskman_unlock(btm), err, err_out,
               "failed to unlock task manager. Possible deadlock!!!\n");
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
            cvec = benesh_get_components(bco);
            if(!cvec) {
                ERR_OUT(BNH_EFAULT, err_out, "failed to get components.");
            }
            BNH_PVEC_FOREACH(comp, bi, cvec)
            {
                CHECK_ZERO(benesh_comp_is_me(comp, &flag), BNH_EFAULT, err_out,
                           "bad component pointer.\n");
                if(!flag) {
                    name = benesh_comp_name(comp);
                    if(!name) {
                        ERR_OUT(BNH_EFAULT, err_out,
                                "failed to get component name.\n");
                    }
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