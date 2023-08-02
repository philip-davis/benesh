#ifndef _BENESH_H_
#define _BENESH_H_

#define _GNU_SOURCE

#include "benesh.h"
#include "ihash.h"
#include "wdmcpl_wrapper.h"
#include "omegah_wrapper.h"
#include "redev_wrapper.h"
#include "xc_config.h"
#include <abt.h>
#include <dspaces.h>
#include <ekt.h>
#include <inttypes.h>
#include <margo.h>
#include <mpi.h>
#include <unistd.h>

#ifdef USE_APEX
#include <apex.h>
#define APEX_FUNC_TIMER_START(fn)                                              \
    apex_profiler_handle profiler0 = apex_start(APEX_FUNCTION_ADDRESS, &fn);
#define APEX_NAME_TIMER_START(num, name)                                       \
    apex_profiler_handle profiler##num = apex_start(APEX_NAME_STRING, name);
#define APEX_TIMER_STOP(num) apex_stop(profiler##num);
#else
#define APEX_FUNC_TIMER_START(fn) (void)0;
#define APEX_NAME_TIMER_START(num, name) (void)0;
#define APEX_TIMER_STOP(num) (void)0;
#endif

#define BENESH_EKT_TP 0
#define BENESH_EKT_WORK 1
#define BENESH_EKT_FINI 2

#undef DEBUG_LOCKS
#undef BDEBUG

#define DEBUG_OUT(dstr, ...)                                                   \
    do {                                                                       \
        if(bnh->f_debug) {                                                     \
            ABT_unit_id tid;                                                   \
            ABT_thread_self_id(&tid);                                          \
            fprintf(                                                           \
                stderr, "Rank %i: TID: %" PRIu64 " %s, line %i (%s): " dstr,   \
                bnh->grank, tid, __FILE__, __LINE__, __func__, ##__VA_ARGS__);  \
        }                                                                      \
    } while(0);

#define DUMMY_OUT(retval) \
    do { \
        if(bnh->dummy) { \
            return retval; \
        } \
    } while(0);

#define BNH_DOM_GRID 1
#define BNH_DOM_MESH 2

struct wf_domain {
    char *name;
    char *full_name;
    int dim;
    union {
        double *lb;
        int class_range[2];
    };
    double *ub;
    double *l_offset;
    uint64_t *l_grid_pts;
    double *l_grid_dims;
    int subdom_count;
    struct wf_domain *subdoms;
    struct omegah_mesh *mesh;
    struct rdv_comm *rdv;
    size_t rdv_dst_count;
    uint32_t *rdv_dest;
    uint32_t *rdv_offset;
    struct rdv_ptn *rptn;
    struct cpl_hndl *cph;
    size_t rdv_count;
    int type;
    int comm_type;
};

struct tpoint_rule {
    size_t nmappings;
    char **map_names;
    char **rule;
    int source;
    struct xc_list_node **tgts;
    int num_tgts;
};

struct tpoint_announce {
    uint32_t rule_id;
    int64_t *tp_vars;
    uint32_t comp_id;
};

struct work_announce {
    uint32_t comp_id;
    uint32_t tgt_id;
    int64_t *tgt_vars;
    int32_t subrule_id;
};

struct tpoint_handle {
    ekt_id ekth;
    struct tpoint_rule *rules;
};

struct pq_obj {
    char **val;
    int len;
};

struct work_node;

struct obj_sub_node {
    struct obj_sub_node *next;
    int done;
    struct work_node *sub;
};

#define BNH_SUB_START 0
#define BNH_SUB_FINISH 1

struct obj_entry {
    struct obj_sub_node *subs;
    int realized;
    int pending;
};

#define BNH_SUBRULE_ASG 0
#define BNH_SUBRULE_PUB 1
#define BNH_SUBRULE_SUB 2
#define BNH_SUBRULE_MTH 3
#define BNH_SUBRULE_MPUB 4
#define BNH_SUBRULE_MSUB 5
#define BNH_SUBRULE_INJ 6

struct var_ver {
    int var_id;
    struct xc_list_node *ver;
};

struct sub_rule {
    int type;
    int comp_id;
    struct xc_expr *expr;
    union {
        int var_id;
        int mth_id;
        char *inj_id;
    };
    int num_invar;
    struct var_ver *invars;
    int num_outvar;
    struct var_ver *outvars;
};

struct wf_target {
    char **obj_name;
    int name_len;
    int num_vars;
    int *tgt_locs;
    char **tgt_vars;
    int ndep;
    struct xc_list_node **deps;
    int num_subrules;
    struct xc_list_node **subrules;
    struct sub_rule *subrule;
};

struct data_sub;

#define BNH_WORK_OBJ 0
#define BNH_WORK_RULE 1
#define BNH_WORK_CHAIN 2
#define BNH_WORK_ANNOUNCE 3
#define BNH_WORK_PENDING 4

struct work_node {
    struct work_node *prev, *next;
    int type;
    union {
        struct wf_target *tgt;
        struct work_node *link;
    };
    int subrule;
    int64_t *var_maps;
    dspaces_sub_t req;
    int announce;
    int realize;
    int sub_req;
    int deps;
    int num_invar;
    struct var_ver *invars;
    int num_outvar;
    struct var_ver *outvars;
    struct data_sub *ds;
};

struct wf_component {
    char *app;
    char *name;
    struct rdv_comm *rdv;
    struct app_hndl *cpl_apph;
    int recv_phase_open;
    int send_phase_open;
    int size;
    int isme;
};

#define BNH_TYPE_INT 0
#define BNH_TYPE_FP 1

#define BNH_COMM_DSP 0
#define BNH_COMM_RDV_SRV 1
#define BNH_COMM_RDV_CLI 2

struct wf_var {
    char *name;
    int type;
    size_t buf_size;
    union {
        double val;
        void *buf;
    };
    struct wf_domain *dom;
    struct xc_int_hash_map *versions;
    struct field_handle **fields;
    int num_fields;
    int comp_id;
    int comm_type;
};

struct wf_method {
    char *name;
    benesh_method method;
    void *arg;
};

struct benesh_handle {
    int rank;
    int grank;
    int comm_size;
    ekt_id ekth;
    MPI_Comm mycomm;
    MPI_Comm gcomm;
    int root_rank;
    int root_drank;
    char *name;
    struct xc_config *conf;
    struct tpoint_handle *tph;
    struct wf_component *comps;
    struct wf_target *tgts;
    struct xc_int_hash_map *known_objs;
    int num_tgts;
    int comp_count;
    int comp_id;
    margo_instance_id mid;
    ABT_mutex work_mutex;
    ABT_cond work_cond;
    ABT_mutex db_mutex;
    ABT_mutex data_mutex;
    ABT_cond data_cond;
    struct work_node *wqueue_head;
    struct work_node *wqueue_tail;
    int gvar_count, ifvar_count;
    struct wf_var *gvars;
    struct wf_var *ifvars;
    int mth_count;
    struct wf_method *mths;
    ekt_type tp_type;
    ekt_type work_type;
    ekt_type fini_type;
    int dom_count;
    struct wf_domain *doms;
    dspaces_client_t dsp;
    int rdvRanks;
    int ready;
    int f_debug;

    int dummy;
    struct wf_domain *dummy_dom;
    struct wf_component *dummy_comp;
    struct wf_var *dummy_vars;
    int num_dummy_vars;
};

static int benesh_get_ipqx_val(struct xc_pqexpr *pqx, int nmappings,
                               char **map_names, int64_t *map_vals, int *val);
static int match_target_rule(struct xc_list_node *obj, struct wf_target *tgt);
static int match_target_rule_fq(struct pq_obj *obj, struct wf_target *tgt,
                                char ***map);
struct wf_domain *match_domain(struct benesh_handle *bnh, const char *dom_name);
void print_work_node(FILE *stream, struct benesh_handle *bnh,
                     struct work_node *wnode);
void print_work_node_nl(FILE *stream, struct benesh_handle *bnh,
                        struct work_node *wnode);
void print_object(FILE *stream, struct wf_target *rule, int64_t *map_vals);
void print_object_nl(FILE *stream, struct wf_target *rule, int64_t *map_vals);

static int signal_status(struct benesh_handle *bnh, int leaving);

int activate_subs(struct benesh_handle *bnh, struct work_node *wnode);
struct wf_var *get_gvar(struct benesh_handle *bnh, const char *name);
struct wf_var *get_ifvar(struct benesh_handle *bnh, const char *name,
                         int comp_id, int *var_id);
int handle_sub(struct benesh_handle *bnh, struct work_node *wnode);
static struct wf_var *match_local_ifvar(struct benesh_handle *bnh, const char *var_name);

static void signal_minmax(struct benesh_handle *bnh, unsigned int signal, int *min, int *max);

struct pq_obj *resolve_obj(struct benesh_handle *bnh, struct xc_list_node *obj, int nmappings, char **map_names, int64_t *vals)
{
    struct pq_obj *res_obj;
    int obj_len = xc_obj_len(obj);
    struct xc_list_node *node;
    char i_str[68];
    int var_loc, str_len, ival;
    int i, j;

    res_obj = malloc(sizeof(*res_obj));
    res_obj->len = obj_len;
    res_obj->val = malloc(sizeof(*res_obj->val) * obj_len);
    for(i = 0, node = obj; node; node = node->next, i++) {
        switch(node->type) {
        case XC_NODE_STR:
            res_obj->val[i] = strdup(node->decl);
            break;
        case XC_NODE_PQVAR:
            for(j = 0, var_loc = -1; j < nmappings; j++) {
                if(strcmp(map_names[j], node->decl) == 0) {
                    var_loc = j;
                    break;
                }
            }
            if(var_loc < 0) {
                fprintf(stderr, "ERROR: unmapped variable found in object.\n");
            } else {
                str_len = sprintf(i_str, "%" PRId64, vals[var_loc]);

                res_obj->val[i] = malloc(str_len + 1);
                sprintf(res_obj->val[i], "%" PRId64, vals[var_loc]);
            }
            break;
        case XC_NODE_PQX:
            if(benesh_get_ipqx_val(node->decl, nmappings, map_names, vals,
                                   &ival) < 0) {
                res_obj->val[i] = strdup("<err>");
            } else {
                str_len = sprintf(i_str, "%i", ival);
                res_obj->val[i] = malloc(str_len + 1);
                sprintf(res_obj->val[i], "%i", ival);
            }
            break;
        default:
            fprintf(stderr, "WARNING: unknown node type during resolution.\n");
        }
    }

    if(bnh->f_debug) {
        DEBUG_OUT("resolved object: %s", res_obj->val[0]);
        for(i = 1; i < res_obj->len; i++) {
            fprintf(stderr, ".%s", res_obj->val[i]);
        }
        fprintf(stderr, "\n");
    }

    return (res_obj);
}

char *obj_atom_tostr(struct wf_target *tgt, int64_t *maps, int pos)
{
    char *res;
    int i;

    if(tgt->obj_name[pos][0] == '%') {
        for(i = 0; i < tgt->num_vars; i++) {
            if(strcmp(&tgt->obj_name[pos][1], tgt->tgt_vars[i]) == 0) {
                asprintf(&res, "%zi", maps[i]);
                return (res);
            }
        }
        return (strdup("<err>"));
    } else {
        return (strdup(tgt->obj_name[pos]));
    }
}

char *wf_target_tostr(struct wf_target *tgt, int64_t *maps)
{
    char *str1, *str2, *res;
    int i;

    res = obj_atom_tostr(tgt, maps, 0);
    for(i = 1; i < tgt->name_len; i++) {
        str1 = res;
        str2 = obj_atom_tostr(tgt, maps, i);
        asprintf(&res, "%s.%s", str1, str2);
        free(str1);
        free(str2);
    }

    return (res);
}

struct obj_entry *get_object_entry(struct benesh_handle *bnh,
                                   struct wf_target *rule, int subrule_id,
                                   int64_t *map_vals, int create)
{
    struct xc_int_hash_map *imap, *parent_imap;
    int rule_id = rule - bnh->tgts;
    struct obj_entry *ent;
    int i;

#ifdef BDEBUG
    fprintf(stderr, "%s for tgt id %li, subrule_id %i\n", __func__,
            rule - bnh->tgts, subrule_id);
#endif

    parent_imap = bnh->known_objs;
    imap = (struct xc_int_hash_map *)xc_ihash_map_lookup(parent_imap, rule_id);
    if(!imap && create) {
        imap = xc_new_ihash_map(16, 1);
        xc_ihash_map_add(parent_imap, rule_id, imap);
    } else if(!imap) {
        return (NULL);
    }

    parent_imap = imap;
    imap =
        (struct xc_int_hash_map *)xc_ihash_map_lookup(parent_imap, subrule_id);
    if(!imap && create) {
        if(rule->num_vars > 0) {
            imap = xc_new_ihash_map(16, 1);
            xc_ihash_map_add(parent_imap, subrule_id, imap);
        } else {
            ent = calloc(1, sizeof(*ent));
            xc_ihash_map_add(parent_imap, subrule_id, ent);
            return (ent);
        }
    } else if(!imap) {
        return (NULL);
    } else if(rule->num_vars == 0) {
        ent = (struct obj_entry *)imap;
        return (ent);
    }

    for(i = 0; i < rule->num_vars; i++) {
        parent_imap = imap;
        imap = (struct xc_int_hash_map *)xc_ihash_map_lookup(parent_imap,
                                                             map_vals[i]);
        if(i < rule->num_vars - 1) {
            if(!imap && create) {
                imap = xc_new_ihash_map(16, 1);
                xc_ihash_map_add(parent_imap, map_vals[i], imap);
            } else if(!imap) {
                return (NULL);
            }
        } else {
            if(imap) {
                ent = (struct obj_entry *)imap;
                return (ent);
            } else if(create) {
                ent = calloc(1, sizeof(*ent));
                xc_ihash_map_add(parent_imap, map_vals[i], ent);
                return (ent);
            } else {
                return (NULL);
            }
        }
    }

    return(NULL);
}

// must enter with db_mutex held!
int sub_target(struct benesh_handle *bnh, struct wf_target *rule, int subtgt_id,
               int64_t *map_vals, struct work_node *sub)
{
    struct obj_entry *ent;
    struct obj_sub_node **subn;
    int sub_added;

    APEX_FUNC_TIMER_START(sub_target);
    if(bnh->f_debug) {
        switch(sub->type) {
        case BNH_WORK_OBJ:
            DEBUG_OUT("Subbing target %li %s to tgt %li, subrule %i\n",
                      sub->tgt - bnh->tgts,
                      ((sub->subrule == 0) ? "realization" : "initiation"),
                      rule - bnh->tgts, subtgt_id);
            break;
        case BNH_WORK_RULE:
            DEBUG_OUT("Subbing target %li, rule %i to tgt %li, rule %i\n",
                      sub->tgt - bnh->tgts, sub->subrule, rule - bnh->tgts,
                      subtgt_id);
            break;
        case BNH_WORK_CHAIN:
            DEBUG_OUT("Subbing a chain to tgt %li, rule %i\n", rule - bnh->tgts,
                      subtgt_id);
            break;
        default:
            fprintf(stderr, "ERROR: subbing unknown type.\n");
        }
    }

    ent = get_object_entry(bnh, rule, subtgt_id, map_vals, 1);
    if(ent->realized) {
        if(bnh->f_debug) {
            DEBUG_OUT("");
            print_object_nl(stderr, rule, map_vals);
            fprintf(stderr, " already realized.\n");
        }
        sub_added = 0;
    } else {
        subn = &ent->subs;
        while(*subn) {
            subn = &(*subn)->next;
        }

        *subn = calloc(1, sizeof(**subn));
        (*subn)->sub = sub;
        sub->deps++;
        sub_added = 1;
    }
    APEX_TIMER_STOP(0);

    return (sub_added);
}

int object_realized(struct benesh_handle *bnh, struct wf_target *rule,
                    int64_t *map_vals)
{
    struct obj_entry *ent;

    ABT_mutex_lock(bnh->db_mutex);

    ent = get_object_entry(bnh, rule, 0, map_vals, 0);
    if(ent && ent->realized) {
        ABT_mutex_unlock(bnh->db_mutex);
        return 1;
    }
    if(bnh->f_debug) {
        char *obj_name = wf_target_tostr(rule, map_vals);
        DEBUG_OUT("object %s (%p) not realized\n", obj_name, (void *)ent);
        free(obj_name);
    }
    ABT_mutex_unlock(bnh->db_mutex);

    return 0;
}

int object_pending(struct benesh_handle *bnh, struct wf_target *rule,
                   int64_t *map_vals)
{
    struct obj_entry *ent;

    ABT_mutex_lock(bnh->db_mutex);

    ent = get_object_entry(bnh, rule, 0, map_vals, 0);
    if(ent && ent->pending) {
        ABT_mutex_unlock(bnh->db_mutex);
        if(bnh->f_debug) {
            DEBUG_OUT(" ");
            print_object(stderr, rule, map_vals);
            fprintf(stderr, " is pending\n");
        }
        return 1;
    }
    ABT_mutex_unlock(bnh->db_mutex);
    return 0;
}

void print_pq_obj(FILE *stream, struct pq_obj *pq)
{
    int i;

    if(pq->len < 1) {
        fprintf(stream, "(empty)");
    }

    fprintf(stream, "%s", pq->val[0]);
    for(i = 1; i < pq->len; i++) {
        fprintf(stream, ".%s", pq->val[i]);
    }
}

void print_pq_obj_nl(FILE *stream, struct pq_obj *pq)
{
    int i;

    if(pq->len < 1) {
        fprintf(stream, "(empty)");
    }

    fprintf(stream, "%s", pq->val[0]);
    for(i = 1; i < pq->len; i++) {
        fprintf(stream, ".%s", pq->val[i]);
    }
    fprintf(stream, "\n");
}

void print_object(FILE *stream, struct wf_target *rule, int64_t *map_vals)
{
    int var_loc;
    int i, j;

    for(i = 0; i < rule->name_len; i++) {
        if(i > 0) {
            fprintf(stream, ".");
        }
        for(j = 0, var_loc = -1; j < rule->num_vars; j++) {
            if(rule->tgt_locs[j] == i) {
                var_loc = j;
                break;
            }
        }
        if(var_loc > -1) {
            fprintf(stream, "%" PRIu64, map_vals[var_loc]);
        } else {
            fprintf(stream, "%s", rule->obj_name[i]);
        }
    }
}

void print_object_nl(FILE *stream, struct wf_target *rule, int64_t *map_vals)
{
    int var_loc;
    int i, j;

    for(i = 0; i < rule->name_len; i++) {
        if(i > 0) {
            fprintf(stream, ".");
        }
        for(j = 0, var_loc = -1; j < rule->num_vars; j++) {
            if(rule->tgt_locs[j] == i) {
                var_loc = j;
                break;
            }
        }
        if(var_loc > -1) {
            fprintf(stream, "%" PRIu64, map_vals[var_loc]);
        } else {
            fprintf(stream, "%s", rule->obj_name[i]);
        }
    }
    fprintf(stream, "\n");
}

void realize_object(struct benesh_handle *bnh, struct wf_target *rule,
                    int64_t *map_vals)
{
    struct obj_entry *ent;
    int i;

    APEX_FUNC_TIMER_START(realize_object);
    if(bnh->f_debug) {
        DEBUG_OUT("realizing object: ");
        print_object_nl(stderr, rule, map_vals);
    }

    APEX_NAME_TIMER_START(1, "db_lock_roa");
    ABT_mutex_lock(bnh->db_mutex);

    ent = get_object_entry(bnh, rule, 0, map_vals, 1);
    DEBUG_OUT(" object entry is %p\n", (void *)ent);
    if(bnh->f_debug) {
        DEBUG_OUT("  rule id = %li\n", rule - bnh->tgts);
        for(i = 0; i < rule->num_vars; i++) {
            DEBUG_OUT("   %s => %" PRIu64 "\n", rule->tgt_vars[i], map_vals[i]);
        }
    }
    if(!ent) {
        fprintf(
            stderr,
            "ERROR: null entry when realizing object (shouldn't happen).\n");
    }
    if(ent->realized) {
        fprintf(
            stderr,
            "WARNING: trying to realize an object that already is realized\n");
    }
    ent->realized = 1;
    ent->pending = 0;
    ABT_mutex_unlock(bnh->db_mutex);
    APEX_TIMER_STOP(0);
}

void obj_set_pending(struct benesh_handle *bnh, struct wf_target *rule,
                     int64_t *map_vals)
{
    struct obj_entry *ent;

#ifdef DEBUG_LOCKS
    fprintf(stderr, "Getting db lock in %s\n", __func__);
#endif
    ABT_mutex_lock(bnh->db_mutex);
#ifdef DEBUG_LOCKS
    fprintf(stderr, "Got db lock in %s\n", __func__);
#endif

    ent = get_object_entry(bnh, rule, 0, map_vals, 1);
    if(!ent) {
        fprintf(stderr,
                "ERROR: null entry when setting pending (shouldn't happen).\n");
    }
    ent->pending = 1;
    ABT_mutex_unlock(bnh->db_mutex);
#ifdef DEBUG_LOCKS
    fprintf(stderr, "Released db lock in %s\n", __func__);
#endif
}

void benesh_make_active(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct work_node *head;

    wnode->prev = NULL;
    wnode->next = bnh->wqueue_head;
    if(bnh->f_debug) {
        DEBUG_OUT("making active ");
        print_work_node_nl(stderr, bnh, wnode);
    }
    if(bnh->wqueue_head) {
        head = bnh->wqueue_head;
        head->prev = wnode;
    }
    bnh->wqueue_head = wnode;
    if(!bnh->wqueue_tail) {
        bnh->wqueue_tail = wnode;
    }
}

int schedule_subrules(struct benesh_handle *bnh, struct wf_target *tgt,
                      int64_t *map_vals, struct work_node *obj_work)
{
    struct work_node *chain, *wnode, **wnodep;
    struct sub_rule *subrule;
    struct obj_entry *ent;
    int i;

    DEBUG_OUT("scheduling subrules for rule %li (%i subrules)\n",
              tgt - bnh->tgts, tgt->num_subrules);

    chain = NULL;
    // lock object database and activity queue for this - dep could come in
    // while we're adding subs
    for(i = 0; i < tgt->num_subrules; i++) {
        subrule = &tgt->subrule[i];
        if(subrule->comp_id == bnh->comp_id) {
            DEBUG_OUT("I have work for target %li, subrule %i\n",
                      tgt - bnh->tgts, i + 1);
            if(chain == NULL) {
                DEBUG_OUT("This subrule is the start of a chain\n");
                chain = calloc(1, sizeof(*chain));
                chain->type = BNH_WORK_CHAIN;
                // NOTE: subtargets start at 1, the target is 0, so decrement
                ABT_mutex_lock(bnh->db_mutex);
                ent = get_object_entry(bnh, tgt, i, map_vals, 1);
                // TODO: delay making active until we're all done, to shrink
                // critical block
                if(ent->realized) {
                    ABT_mutex_unlock(bnh->db_mutex);
                    // we already have the work lock
                    DEBUG_OUT(
                        "Previous subrule already realized; the new chain "
                        "should go in the work queue immediately.\n");
                    benesh_make_active(bnh, chain);
                } else {
                    // i == 0 means object initialization
                    if(bnh->f_debug) {
                        if(i) {
                            DEBUG_OUT("Subscribe the new chain to target %li, "
                                      "subrule %i\n",
                                      tgt - bnh->tgts, i);
                        } else {
                            DEBUG_OUT("Subscribe the new chain to target %li "
                                      "initiation\n",
                                      tgt - bnh->tgts);
                        }
                    }
                    sub_target(bnh, tgt, (i ? i : -1), map_vals, chain);
                    ABT_mutex_unlock(bnh->db_mutex);
                }
                wnodep = &chain->link;
                wnode = chain;
            }
            *wnodep = calloc(1, sizeof(**wnodep));
            (*wnodep)->prev = wnode;
            wnode = *wnodep;
            wnode->type = BNH_WORK_RULE;
            wnode->tgt = tgt;
            wnode->subrule = i + 1;
            wnode->var_maps = map_vals;
            wnodep = &wnode->next;
            if(tgt->subrule[i].type == BNH_SUBRULE_SUB) {
                /* This should be done here so we can overlap the waiting and
                 * transfer with other work */
                handle_sub(bnh, wnode);
            }
        } else {
            if(chain) {
                DEBUG_OUT("subrule %i should announce completion\n",
                          wnode->subrule);
                wnode->announce = 1;
                chain = NULL;
            }
        }
    }

    return (subrule->comp_id);
}

struct wf_target *find_target_rule(struct benesh_handle *bnh,
                                   struct pq_obj *obj, int64_t **map_vals)
{
    struct wf_target *tgt_rule;
    char **map;
    int i, j;

    // TODO memoize
    for(i = 0; i < bnh->num_tgts; i++) {
        if(match_target_rule_fq(obj, &bnh->tgts[i], &map)) {
            tgt_rule = &bnh->tgts[i];
            *map_vals = malloc(sizeof(**map_vals) * tgt_rule->num_vars);
            for(j = 0; j < tgt_rule->num_vars; j++) {
                (*map_vals)[j] = atoi(map[j]);
            }
            free(map);
            return (tgt_rule);
        }
    }

    fprintf(stderr, "ERROR: no target match found for object.\n");
    return (NULL);
}

int schedule_target(struct benesh_handle *bnh, struct pq_obj *tgt)
{
    struct wf_target *tgt_rule, *dep_tgt_rule;
    struct pq_obj **dep_tgts = NULL;
    struct work_node *obj_work_init, *obj_work_fini;
    int64_t *map_vals = NULL, *dep_map_vals;
    int realized, dep_remain = 0;
    int i;

    APEX_FUNC_TIMER_START(schedule_target);
    for(i = 0; i < tgt->len; i++) {
        if(tgt->val[i][0] == '%') {
            fprintf(stderr,
                    "ERROR: trying to schedule an unresolved target.\n");
            return -1;
        }
    }

    tgt_rule = find_target_rule(bnh, tgt, &map_vals);
    if(bnh->f_debug) {
        DEBUG_OUT("scheduling ");
        print_pq_obj(stderr, tgt);
        DEBUG_OUT(" as rule %li\n", tgt_rule - bnh->tgts);
    }

    realized = 0;
    if(object_realized(bnh, tgt_rule, map_vals)) {
        if(bnh->f_debug) {
            print_pq_obj(stderr, tgt);
            DEBUG_OUT(" already realized.\n");
        }
        realized = 1;
    } else if(!object_pending(bnh, tgt_rule, map_vals)) {
        obj_set_pending(bnh, tgt_rule, map_vals);
        obj_work_init = calloc(1, sizeof(*obj_work_init));
        obj_work_init->type = BNH_WORK_OBJ;
        obj_work_init->tgt = tgt_rule;
        obj_work_init->subrule = -1;
        obj_work_init->var_maps = map_vals;
        // pare this down later - requires some configuration inspection
        obj_work_init->announce = 0;
        dep_tgts = malloc(sizeof(*dep_tgts) * tgt_rule->ndep);
        dep_remain = 0;
        if(bnh->f_debug) {
            DEBUG_OUT("checking dependencies for ");
            print_pq_obj_nl(stderr, tgt);
        }
        for(i = 0; i < tgt_rule->ndep; i++) {
            dep_tgts[i] =
                resolve_obj(bnh, tgt_rule->deps[i], tgt_rule->num_vars,
                            tgt_rule->tgt_vars, map_vals);
            if(!schedule_target(bnh, dep_tgts[i])) {
                if(bnh->f_debug) {
                    DEBUG_OUT("");
                    print_pq_obj(stderr, dep_tgts[i]);
                    fprintf(stderr, ", dependency %i of ", i);
                    print_pq_obj(stderr, tgt);
                    fprintf(stderr, ", not realized yet.\n");
                }
                dep_tgt_rule =
                    find_target_rule(bnh, dep_tgts[i], &dep_map_vals);
                ABT_mutex_lock(bnh->db_mutex);
                dep_remain += sub_target(bnh, dep_tgt_rule, 0, dep_map_vals, obj_work_init);
                ABT_mutex_unlock(bnh->db_mutex);
            } else {
                if(bnh->f_debug) {
                    DEBUG_OUT("");
                    print_pq_obj(stderr, dep_tgts[i]);
                    fprintf(stderr, ", dependency %i of ", i);
                    print_pq_obj(stderr, tgt);
                    fprintf(stderr, " already realized\n");
                }
             }
        }
        if(tgt_rule->num_subrules > 0) {
            if(schedule_subrules(bnh, tgt_rule, map_vals, obj_work_init) ==
               bnh->comp_id) {
                // to send the completion
                DEBUG_OUT("I will announced finalization of target %li\n", tgt_rule - bnh->tgts);
                obj_work_fini = calloc(1, sizeof(*obj_work_fini));
                obj_work_fini->type = BNH_WORK_OBJ;
                obj_work_fini->tgt = tgt_rule;
                obj_work_fini->subrule = 0;
                obj_work_fini->var_maps = map_vals;
                obj_work_fini->announce = 1;
                obj_work_fini->realize = 1;
                ABT_mutex_lock(bnh->db_mutex);
                sub_target(bnh, tgt_rule, tgt_rule->num_subrules, map_vals,
                           obj_work_fini);
                ABT_mutex_unlock(bnh->db_mutex);
            }
        } else {
            obj_work_init->subrule = 0;
            obj_work_init->realize = 1;
        }

        if(dep_remain == 0) {
             if(bnh->f_debug) {
                DEBUG_OUT("all dependencies already met while scheduling ");
                print_pq_obj_nl(stderr, tgt);
            }
            // we already have the work lock
            benesh_make_active(bnh, obj_work_init);
        }
    }

    // We save this in work_nodes. Might want to do something different.
    /*
        if(tgt_rule->num_vars > 0) {
            free(map_vals);
        }
    */
    if(tgt_rule->ndep > 0) {
        free(dep_tgts);
    }

    APEX_TIMER_STOP(0);
    return (realized);
}

static int work_watch(void *work_v, void *bnh_v)
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
    // activate_subs only signals the handler if some sub is satsified. This object being
    // realized may mean we are done with a touchpoint.
    DEBUG_OUT("Signalling handler to restart\n");
    ABT_cond_signal(bnh->work_cond);
    APEX_TIMER_STOP(0);

    return (0);
}

char *tpoint_tostr(const char *comp_name, struct tpoint_rule *rule)
{
    char **token;
    char *str;
    int len;

    len = strlen(comp_name) + 2;
    for(token = rule->rule; *token; token++) {
        len += strlen(*token) + 1;
    }

    str = malloc(len);
    strcpy(str, comp_name);
    strcat(str, "@");
    strcat(str, rule->rule[0]);
    for(token = &rule->rule[1]; *token; token++) {
        strcat(str, ".");
        strcat(str, *token);
    }

    return (str);
}

static int tpoint_watch(void *tpoint_v, void *bnh_v)
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
            DEBUG_OUT("     [%s] => %" PRId64 "\n", rule->map_names[i], tpoint->tp_vars[i]);
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

static int serialize_work(void *work_v, void *bnh_v, void **buf)
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

static int serialize_tpoint(void *tpoint_v, void *bnh_v, void **buf)
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

static int deserialize_tpoint(void *buf, void *bnh_v, void **tpoint_v)
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

static int deserialize_work(void *buf, void *bnh_v, void **work_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    struct work_announce *work = malloc(sizeof(*work));
    struct wf_target *tgt;
    size_t nmap;

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

static int serialize_fini(void *fini_v, void *bnh_v, void **buf)
{
    uint32_t *comp_id = (uint32_t *)fini_v;

    *buf = malloc(sizeof(*comp_id));
    *(uint32_t *)(*buf) = *comp_id;

    return (sizeof(*comp_id));
}

static int deserialize_fini(void *buf, void *bnh_v, void **fini_v)
{
    uint64_t *comp_id = malloc(sizeof(*comp_id));

    *comp_id = *(uint32_t *)buf;

    *fini_v = comp_id;

    return 0;
}

static int fini_watch(void *fini_v, void *bnh_v)
{
    struct benesh_handle *bnh = (struct benesh_handle *)bnh_v;
    uint32_t comp_id = *(uint32_t *)fini_v;

    ABT_mutex_lock(bnh->work_mutex);
    bnh->comp_count--;
    if(bnh->f_debug) {
        DEBUG_OUT("Got finalize from component %i. %i remaining\n", comp_id, bnh->comp_count);
    }
    ABT_cond_signal(bnh->work_cond);
    ABT_mutex_unlock(bnh->work_mutex);

    return 0;
}

int bnsh_tpoint_init(struct benesh_handle *bnh, struct tpoint_rule *tp_rules,
                     struct tpoint_handle **tph)
{
    *tph = malloc(sizeof(**tph));
    (*tph)->ekth = bnh->ekth;
    (*tph)->rules = tp_rules;

    return 0;
}

int bnsh_tpoint_fini(struct tpoint_handle *tph)
{
    // ekt_deregister(&tph->type);
    free(tph);

    return (0);
}

int bnsh_tpoint_announce(struct benesh_handle *bnh, int rule, int64_t *values)
{
    struct tpoint_announce announce = {rule, values};

    return(ekt_tell(bnh->ekth, NULL, bnh->tp_type, &announce));
}

static char **tokenize_tpoint(const char *tpoint, int *tkcnt)
{
    char *savep;
    char *tpoint_str;
    char **tokenized;
    char *token;
    int i;

    if(!tpoint || strlen(tpoint) == 0) {
        *tkcnt = 0;
        return (NULL);
    }

    tpoint_str = strdup(tpoint);

    *tkcnt = 1;
    for(i = 0; i < strlen(tpoint); i++) {
        if(tpoint[i] == '.') {
            (*tkcnt)++;
        }
    }

    tokenized = malloc(sizeof(*tokenized) * *tkcnt);
    token = strtok_r(tpoint_str, ".", &savep);
    tokenized[0] = strdup(token);
    for(i = 1; i < *tkcnt; i++) {
        token = strtok_r(NULL, ".", &savep);
        tokenized[i] = strdup(token);
    }

    free(tpoint_str);

    return (tokenized);
}

static int get_rule_size(struct tpoint_rule *rule)
{
    char **token;
    int size = 0;

    for(token = rule->rule; *token; token++) {
        size++;
    }

    return (size);
}

static int match_rule(struct tpoint_rule *rule, char **tk_tpoint, int tkcnt,
                      int64_t **values)
{
    int mappos[rule->nmappings];
    int rsize = get_rule_size(rule);
    int i, j;

    if(rsize != tkcnt) {
        return 0;
    }

    for(i = 0; i < rsize; i++) {
        if(rule->rule[i][0] == '%') {
            for(j = 0; j < rule->nmappings; j++) {
                if(strcmp(&rule->rule[i][1], rule->map_names[j]) == 0) {
                    mappos[j] = i;
                    break;
                }
            }
        } else if(strcmp(rule->rule[i], tk_tpoint[i]) != 0) {
            return 0;
        }
    }

    *values = malloc(rule->nmappings * sizeof(**values));
    for(i = 0; i < rule->nmappings; i++) {
        (*values)[i] = atoi(tk_tpoint[mappos[i]]);
    }

    return (1);
}

static void benesh_load_config(struct benesh_handle *bnh, const char *conf)
{
    if(bnh->rank == 0) {
        DEBUG_OUT("reading workflow configuration from %s...\n", conf);
    }
    APEX_FUNC_TIMER_START(benesh_load_config);
    bnh->conf = xc_fparse(conf, bnh->gcomm);
    APEX_TIMER_STOP(0);
}

// Deserialize tpoint nodes from conf
struct tpoint_rule *get_tpoint_rules(struct benesh_handle *bnh)
{
    struct xc_config *conf = bnh->conf;
    struct xc_list_node **tpnodes, *rnode, *tgt_obj;
    struct xc_tprule *tprule;
    struct tpoint_rule *rules, *tgt_rule;
    struct xc_component *comp;
    int rule_len;
    int rule_count, map_count;
    char *var_str;
    int i, j;

    tpnodes = xc_get_all(conf->subconf, XC_NODE_TPRULE, &rule_count);
    rules = calloc((rule_count + 1), sizeof(*rules));
    for(i = 0; i < rule_count; i++) {
        tprule = (struct xc_tprule *)tpnodes[i]->decl;
        rule_len = xc_obj_len(tprule->tpoint);
        tgt_rule = &rules[i];
        tgt_rule->rule = calloc(sizeof(*(tgt_rule->rule)), rule_len + 1);
        for(j = 0, rnode = tprule->tpoint; j < rule_len;
            j++, rnode = rnode->next) {
            xc_node_tostr(rnode, &tgt_rule->rule[j]);
            if(rnode->type == XC_NODE_PQVAR) {
                var_str = malloc(strlen(tgt_rule->rule[j]) + 2);
                strcpy(&var_str[1], tgt_rule->rule[j]);
                var_str[0] = '%';
                free(tgt_rule->rule[j]);
                tgt_rule->rule[j] = var_str;
                tgt_rule->nmappings++;
            }
        }
        tgt_rule->rule[rule_len] = NULL;
        if(tgt_rule->nmappings) {
            tgt_rule->map_names =
                calloc(sizeof(*tgt_rule->map_names), tgt_rule->nmappings);
            map_count = 0;
            for(rnode = tprule->tpoint; rnode; rnode = rnode->next) {
                if(rnode->type == XC_NODE_PQVAR) {
                    xc_node_tostr(rnode, &tgt_rule->map_names[map_count++]);
                }
            }
        }
        comp = tprule->comp;
        if(strcmp(comp->app, bnh->name) == 0) {
            tgt_rule->source = 1;
        }
        for(tgt_obj = tprule->obj; tgt_obj; tgt_obj = tgt_obj->next) {
            tgt_rule->num_tgts++;
        }
        tgt_rule->tgts = malloc(tgt_rule->num_tgts * sizeof(*tgt_rule->tgts));
        for(j = 0, tgt_obj = tprule->obj; j < tgt_rule->num_tgts;
            j++, tgt_obj = tgt_obj->next) {
            tgt_rule->tgts[j] = tgt_obj->decl;
        }
    }
    rules[rule_count].rule = NULL;
    rules[rule_count].nmappings = 0;
    rules[rule_count].map_names = NULL;

    free(tpnodes);

    return (rules);
}

static struct wf_domain *match_conf_domain(struct benesh_handle *bnh,
                                           struct xc_domain *domain)
{
    struct wf_domain *wf_dom;
    int i;

    if(!domain->parent) {
        for(i = 0; i < bnh->dom_count; i++) {
            if(strcmp(bnh->doms[i].name, domain->name) == 0) {
                return (&bnh->doms[i]);
            }
        }
        return NULL;
    } else {
        wf_dom = match_conf_domain(bnh, domain->parent);
        if(!wf_dom) {
            return (NULL);
        }
        for(i = 0; i < wf_dom->subdom_count; i++) {
            if(strcmp(wf_dom->subdoms[i].name, domain->name) == 0) {
                return (&wf_dom->subdoms[i]);
            }
        }
        return (NULL);
    }
}

static void benesh_init_comps(struct benesh_handle *bnh)
{
    struct xc_config *conf = bnh->conf;
    struct xc_list_node **compnodes, *node;
    struct xc_component *comp;
    int comp_count;
    struct xc_iface *iface;
    struct xc_list_node **mthnodes, **varnodes, **dmapnodes;
    int mth_count, var_count, dmap_count;
    struct xc_conf_method *mth;
    struct xc_conf_var *var;
    struct xc_domain_map *dmap;
    struct wf_var *wf_var, *if_var;
    struct xc_dmap_attr *dmattr;
    char *var_name;
    int found;
    int i, j;

    compnodes = xc_get_all(conf->subconf, XC_NODE_COMP, &comp_count);
    DEBUG_OUT("%i components in workflow\n", comp_count);
    bnh->comp_count = comp_count;
    bnh->comps = calloc(sizeof(*(bnh->comps)), comp_count);
    for(i = 0, bnh->ifvar_count = 0; i < comp_count; i++) {
        comp = compnodes[i]->decl;
        iface = comp->iface;
        varnodes = xc_get_all(iface->decl, XC_NODE_VAR, &var_count);
        bnh->ifvar_count += var_count;
        free(varnodes);
    }
    bnh->ifvars = calloc(bnh->ifvar_count, sizeof(*bnh->ifvars));
    for(i = 0, if_var = bnh->ifvars; i < comp_count; i++) {
        comp = compnodes[i]->decl;
        bnh->comps[i].app = comp->app;
        bnh->comps[i].name = comp->name;
        iface = comp->iface;
        varnodes = xc_get_all(iface->decl, XC_NODE_VAR, &var_count);
        for(j = 0; j < var_count; j++) {
            var = varnodes[j]->decl;
            if_var[j].name = var->name;
            if_var[j].versions = xc_new_ihash_map(32, 1);
            if_var[j].comp_id = i;
            if(i == bnh->comp_id) {
                if_var[j].versions = xc_new_ihash_map(32, 1);
            }
            if_var[j].comm_type = BNH_COMM_DSP;
        }
        free(varnodes);
        if_var += var_count;
        DEBUG_OUT("We are %s\n", bnh->name);
        if(strcmp(comp->app, bnh->name) != 0) {
            if(!bnh->dummy) {
                DEBUG_OUT("connecting to component %i (%s)\n", i, comp->app);
                ekt_connect(bnh->ekth, comp->app);
                if(strcmp(comp->name, "Coupler") == 0) {
                    DEBUG_OUT("We are talking to rdv\n");
                    bnh->rdvRanks = ekt_peer_size(bnh->ekth, comp->app);
                } else if(strstr(comp->name, "Client") == 0) {
                    DEBUG_OUT("We are rdv\n");
                    bnh->rdvRanks = bnh->comm_size;
                }
            }
            MPI_Bcast(&bnh->rdvRanks, 1, MPI_INT, bnh->root_rank, bnh->gcomm);
            if(bnh->rdvRanks) {
                DEBUG_OUT("%i rendezvous ranks\n", bnh->rdvRanks);
            }
        } else {
            bnh->comps[i].isme = 1;
            bnh->comp_id = i;
            mthnodes = xc_get_all(iface->decl, XC_NODE_METHOD, &mth_count);
            bnh->mths = calloc(mth_count, sizeof(*bnh->mths));
            bnh->mth_count = mth_count;
            for(j = 0; j < mth_count; j++) {
                mth = mthnodes[j]->decl;
                bnh->mths[j].name = mth->name;
            }
            free(mthnodes);
        }
    }
    free(compnodes);

    dmapnodes = xc_get_all(conf->subconf, XC_NODE_DMAP, &dmap_count);
    DEBUG_OUT("found %i domain maps\n", dmap_count);
    for(i = 0, found = 0; i < dmap_count; i++) {
        dmap = dmapnodes[i]->decl;
        // TODO domain assignments can be more complicated
        node = dmap->obj->next;
        var_name = node->decl;
        for(j = 0, found = 0; j < comp_count; j++) {
            if(strcmp(bnh->comps[j].name, dmap->obj->decl) == 0) {
                wf_var = get_ifvar(bnh, var_name, j, NULL);
                found = 1;
                break;
            }
        }
        if(!found) {
            fprintf(stderr, "ERROR: no component %s for domain map.\n",
                    (char *)dmap->obj->decl);
        } else {
            wf_var->dom = match_conf_domain(bnh, dmap->domain);
            if(dmap->attrs) {
                node = dmap->attrs;
                dmattr = node->decl;
                if(strcmp(dmattr->val, "rdv_server") == 0) {
                    if(wf_var->dom->comm_type == BNH_COMM_RDV_CLI) {
                        fprintf(stderr,
                                "WARNING: %s being marked rdv server, already "
                                "client.\n",
                                wf_var->dom->full_name);
                    }
                    wf_var->dom->comm_type = BNH_COMM_RDV_SRV;
                    DEBUG_OUT("component %s is rdv-server for var '%s'\n",
                              bnh->comps[wf_var->comp_id].name, wf_var->name);
                } else if(strcmp(dmattr->val, "rdv_client") == 0) {
                    if(wf_var->dom->comm_type == BNH_COMM_RDV_SRV) {
                        fprintf(stderr,
                                "WARNING: %s being marked rdv client, already "
                                "server.\n",
                                wf_var->dom->full_name);
                    }
                    wf_var->dom->comm_type = BNH_COMM_RDV_CLI;
                    DEBUG_OUT("component %s is rdv-client for var '%s'\n",
                              bnh->comps[wf_var->comp_id].name, wf_var->name);
                } else {
                    fprintf(
                        stderr,
                        "WARNING: unimplemented domain map attribute '%s'\n",
                        (char *)dmattr->val);
                }
            }
        }
    }
    free(dmapnodes);
}

static void load_domain(struct benesh_handle *bnh, struct xc_list_node *dnode,
                        struct wf_domain *dom, const char *prefix)
{
    struct xc_domain *conf_dom = dnode->decl;
    struct xc_list_node *node, **attrnodes, **domnodes;
    int attr_count;
    struct xc_conf_attr *attr;
    int range_found = 0;
    struct xc_ival *ival;
    int i, j;

    DEBUG_OUT("loading domain %s with prefix %s\n", conf_dom->name, prefix);

    if(prefix) {
        dom->full_name = malloc(strlen(prefix) + strlen(conf_dom->name) + 2);
        dom->full_name[0] = '\0';
        strcat(dom->full_name, prefix);
        strcat(dom->full_name, ".");
    } else {
        dom->full_name = malloc(strlen(conf_dom->name) + 1);
        dom->full_name[0] = '\0';
    }
    strcat(dom->full_name, conf_dom->name);
    dom->name = conf_dom->name;
    attrnodes = xc_get_all(conf_dom->decl, XC_NODE_ATTR, &attr_count);
    dom->type = BNH_DOM_GRID;
    for(i = 0; i < attr_count; i++) {
        attr = attrnodes[i]->decl;
        if(strcmp(attr->name, "range") == 0) {
            node = attr->val;
            dom->dim = xc_obj_len(node);
            dom->lb = malloc(sizeof(*dom->lb) * dom->dim);
            dom->ub = malloc(sizeof(*dom->ub) * dom->dim);
            for(j = 0; j < dom->dim; j++) {
                ival = node->decl;
                dom->lb[j] = ival->min;
                dom->ub[j] = ival->max;
                node = node->next;
            }
            range_found = 1;
            break;
        } else if(strcmp(attr->name, "mesh") == 0) {
            dom->type = BNH_DOM_MESH;
        } else if(strcmp(attr->name, "class") == 0) {
            dom->type = BNH_DOM_MESH;
            node = attr->val;
            ival = node->decl;
            dom->class_range[0] = ival->idmin;
            dom->class_range[1] = ival->idmax;
        }
    }
    if(!range_found && dom->type == BNH_DOM_GRID) {
        fprintf(stderr, "ERROR: no range found for domain.\n");
    }
    domnodes = xc_get_all(conf_dom->decl, XC_NODE_DOM, &dom->subdom_count);
    if(dom->subdom_count > 0) {
        dom->subdoms = calloc(sizeof(*dom->subdoms), dom->subdom_count);
        for(i = 0; i < dom->subdom_count; i++) {
            load_domain(bnh, domnodes[i], &dom->subdoms[i], dom->full_name);
        }
    }
}

static void benesh_init_doms(struct benesh_handle *bnh)
{
    struct xc_config *conf = bnh->conf;
    struct xc_list_node **domnodes;
    struct wf_domain *dom;
    int dom_count;
    int i;

    domnodes = xc_get_all(conf->subconf, XC_NODE_DOM, &dom_count);

    bnh->dom_count = dom_count;
    bnh->doms = calloc(dom_count, sizeof(*bnh->doms));
    for(i = 0; i < dom_count; i++) {
        dom = &bnh->doms[i];
        load_domain(bnh, domnodes[i], dom, NULL);
    }
}

static void benesh_init_vars(struct benesh_handle *bnh)
{
    struct xc_config *conf = bnh->conf;
    struct xc_list_node **varnodes;
    struct xc_conf_var *var;
    int var_count;
    int i;

    varnodes = xc_get_all(conf->subconf, XC_NODE_VAR, &var_count);
    bnh->gvar_count = var_count;
    bnh->gvars = malloc(sizeof(*bnh->gvars) * var_count);
    for(i = 0; i < var_count; i++) {
        var = (struct xc_conf_var *)varnodes[i]->decl;
        bnh->gvars[i].name = var->name;
        if(strcmp(var->base->name, "integer") == 0) {
            bnh->gvars[i].type = BNH_TYPE_INT;
        } else if(strcmp(var->base->name, "real") == 0) {
            bnh->gvars[i].type = BNH_TYPE_FP;
        } else {
            fprintf(stderr,
                    "ERROR: unsupported base type for workflow variable.\n");
        }
        if(var->card->type != XC_CARD_SCALAR) {
            fprintf(stderr, "WARNING: unsupported cardinality...ignorning.\n");
        }
        if(var->val) {
            // TODO: be more sophisticated about types
            bnh->gvars[i].val = *(double *)var->val;
        } else {
            bnh->gvars[i].val = 0;
        }
    }

    free(varnodes);
}

static int benesh_get_ipqx_val(struct xc_pqexpr *pqx, int nmappings,
                               char **map_names, int64_t *map_vals, int *val)
{
    // TODO: extend!!!
    /*
        if(pqx->type != XC_PQ_INT) {
            fprintf(
                stderr,
                "ERROR: tried to get integer value of non-integer
       expression.\n"); return(-1);
        }

        return (*(int *)pqx->val);
        *val =

        return(0);
    */
    int lval, rval, sign = 1;
    int i;

    switch(pqx->type) {
    case XC_PQ_INT:
        *val = *(int *)pqx->val;
        return (0);
    case XC_PQ_REAL:
        fprintf(stderr, "ERROR: expected integer in expression, saw real.\n");
        return (-1);
    case XC_PQ_SUB:
        sign = -1;
    case XC_PQ_ADD:
        if((benesh_get_ipqx_val(pqx->lside, nmappings, map_names, map_vals,
                                &lval) == 0) &&
           (benesh_get_ipqx_val(pqx->rside, nmappings, map_names, map_vals,
                                &rval) == 0)) {
            *val = lval + (sign * rval);
            return (0);
        } else {
            return (-1);
        }
    case XC_PQ_VAR:
        for(i = 0; i < nmappings; i++) {
            if(strcmp(map_names[i], pqx->val) == 0) {
                *val = map_vals[i];
                return (0);
            }
        }
        fprintf(stderr, "unknown mapping '%s' in expression.\n",
                (char *)pqx->val);
        return (-1);
    default:
        fprintf(stderr, "ERROR: unknown expression type!\n");
        return (-1);
    }

    return (0);
}

static int match_target_rule_fq(struct pq_obj *obj, struct wf_target *tgt,
                                char ***map)
{
    char **btoamap;
    int var_loc;
    int match = 1;
    int i, j;

    *map = NULL;
    if(tgt->name_len != obj->len) {
        return 0;
    }

    btoamap = calloc(sizeof(*btoamap), tgt->name_len);
    for(i = 0; i < tgt->name_len; i++) {
        var_loc = -1;
        for(j = 0; j < tgt->num_vars; j++) {
            if(tgt->tgt_locs[j] == i) {
                var_loc = j;
            }
        }
        if(var_loc < 0) {
            if(strcmp(obj->val[i], tgt->obj_name[i]) != 0) {
                match = 0;
            }
        } else if(btoamap[var_loc]) {
            if(strcmp(btoamap[var_loc], obj->val[i]) != 0) {
                match = 0;
            }
        } else {
            btoamap[var_loc] = obj->val[i];
        }
        if(match == 0) {
            break;
        }
    }

    if(match) {
        *map = btoamap;
    } else {
        free(btoamap);
    }

    return (match);
}

static int match_target_rule(struct xc_list_node *obj, struct wf_target *tgt)
{
    struct xc_list_node *node;
    char **atobmap, **btoamap;
    int *obj_var_locs;
    int obj_len, amaplen = 0, bmaplen;
    int var_loc, obj_var_pos;
    int match = 1;
    int ival, tval;
    int i, j;

    obj_len = xc_obj_len(obj);
    if(obj_len != tgt->name_len) {
        return (0);
    }

    for(node = obj; node; node = node->next) {
        if(node->type == XC_NODE_PQVAR) {
            amaplen++;
        }
    }
    bmaplen = tgt->num_vars;
    atobmap = calloc(amaplen, sizeof(*atobmap));
    obj_var_locs = calloc(amaplen, sizeof(*obj_var_locs));
    for(i = 0, node = obj, var_loc = 0; node; i++, node = node->next) {
        if(node->type == XC_NODE_PQVAR) {
            obj_var_locs[var_loc++] = i;
        }
    }
    btoamap = calloc(bmaplen, sizeof(*btoamap));
    for(node = obj, i = 0; i < obj_len; node = node->next, i++) {
        var_loc = -1;
        for(j = 0; j < bmaplen; j++) {
            if(tgt->tgt_locs[j] == i) {
                var_loc = j;
            }
        }
        switch(node->type) {
        case XC_NODE_STR:
            if(var_loc == -1) {
                // this part name in the target is a string
                if(strcmp(node->decl, tgt->obj_name[i]) != 0) {
                    match = 0;
                }
            } else if(!btoamap[var_loc]) {
                // this part name in the target is a variable
                btoamap[var_loc] = node->decl;
            } else {
                if(strcmp(btoamap[var_loc], node->decl) != 0) {
                    match = 0;
                }
            }
            break;
        case XC_NODE_PQVAR:
            if(var_loc == -1) {
                for(j = 0; j < amaplen; j++) {
                    if(obj_var_locs[j] == i) {
                        obj_var_pos = j;
                        break;
                    }
                }
                if(!atobmap[obj_var_pos]) {
                    atobmap[obj_var_pos] = tgt->obj_name[i];
                } else {
                    if(strcmp(atobmap[obj_var_pos], tgt->obj_name[i]) != 0) {
                        match = 0;
                    }
                }
            }
            break;
        case XC_NODE_PQX:
            if(benesh_get_ipqx_val(node->decl, 0, NULL, NULL, &ival) < 0) {
                match = 0;
                break;
            }
            if(var_loc == -1) {
                tval = atoi(tgt->obj_name[i]);
                if(ival != tval) {
                    match = 0;
                }
            } else {
                xc_node_tostr(node, &atobmap[var_loc]);
            }
            break;
        default:
            fprintf(stderr, "ERROR: bad node type in rule matching\n");
        }
        if(match == 0) {
            break;
        }
    }

    free(atobmap);
    free(btoamap);
    free(obj_var_locs);

    return (match);
}

static int benesh_find_target(struct benesh_handle *bnh,
                              struct xc_list_node *obj)
{
    int result = -1;
    int dep_found = 0;
    int i;

    dep_found = 0;
    for(i = 0; i < bnh->num_tgts; i++) {
        if(match_target_rule(obj, &bnh->tgts[i])) {
            if(dep_found) {
                fprintf(stderr, "WARNING: ambiguous target dependency!\n");
            } else {
                dep_found = 1;
                result = i;
                break; // targets are matched in order
            }
        }
    }

    return (result);
}

static int find_comp_idx_by_name(struct benesh_handle *bnh, const char *name)
{
    int i;

    for(i = 0; i < bnh->comp_count; i++) {
        if(strcmp(bnh->comps[i].name, name) == 0) {
            return(i);
        }
    }

    return(-1);
}

static int benesh_load_targets(struct benesh_handle *bnh)
{
    struct xc_list_node **tgtnodes, *node, *arg_node, *pnode, *vvnode;
    struct wf_target *wtgt;
    int tgt_count;
    struct xc_config *conf = bnh->conf;
    struct xc_target *tgt;
    struct xc_expr *expr;
    struct xc_minst *minst;
    struct xc_obj_fusion *fus;
    struct xc_vmap *vmap;
    struct xc_varver *vv;
    int tgt_len;
    char *var_str;
    char *comp_name;
    int str_len;
    int var_pos = 0;
    int found;
    int i, j, k;

    tgtnodes = xc_get_all(conf->subconf, XC_NODE_TGTRULE, &tgt_count);

    bnh->num_tgts = tgt_count;
    bnh->tgts = calloc(tgt_count, sizeof(*bnh->tgts));
    for(i = 0; i < tgt_count; i++) {
        wtgt = &bnh->tgts[i];
        tgt = tgtnodes[i]->decl;
        tgt_len = xc_obj_len(tgt->tgtobj);
        wtgt->obj_name = calloc(tgt_len, sizeof(char *));
        for(j = 0, node = tgt->tgtobj; j < tgt_len; j++, node = node->next) {
            if(node->type == XC_NODE_STR || node->type == XC_NODE_PQX) {
                xc_node_tostr(node, &wtgt->obj_name[j]);
            } else if(node->type == XC_NODE_PQVAR) {
                var_str = NULL;
                str_len = xc_node_tostr(node, &var_str);
                wtgt->obj_name[j] = malloc(sizeof(str_len + 2));
                wtgt->obj_name[j][0] = '%';
                strcpy(&wtgt->obj_name[j][1], var_str);
                free(var_str);
                wtgt->num_vars++;
            } else {
                fprintf(stderr, "WARNING: unimplemented node type.\n");
            }
        }
        wtgt->name_len = tgt_len;
        wtgt->tgt_vars = calloc(wtgt->num_vars, sizeof(char *));
        wtgt->tgt_locs = calloc(wtgt->num_vars, sizeof(int *));
        var_pos = 0;
        for(j = 0, node = tgt->tgtobj; j < tgt_len; j++, node = node->next) {
            if(node->type == XC_NODE_PQVAR) {
                xc_node_tostr(node, &wtgt->tgt_vars[var_pos]);
                wtgt->tgt_locs[var_pos++] = j;
            }
        }

        wtgt->ndep = 0;
        for(node = tgt->deps; node; node = node->next) {
            wtgt->ndep++;
        }
        wtgt->deps = malloc(wtgt->ndep * sizeof(*wtgt->deps));
        for(j = 0, node = tgt->deps; node; j++, node = node->next) {
            wtgt->deps[j] = node->decl;
        }

        wtgt->num_subrules = 0;
        for(node = tgt->procedure; node; node = node->next) {
            expr = node->decl;
            if(expr->type == XC_EXPR_MXFR) {
                // two pubs and a sub
                wtgt->num_subrules += 3;
            }else if(expr->type == XC_EXPR_XFR) {
                // A pub and a sub
                wtgt->num_subrules += 2;
            } else {
                wtgt->num_subrules++;
            }
        }
        wtgt->subrule = calloc(wtgt->num_subrules, sizeof(*wtgt->subrule));
        for(j = 0, node = tgt->procedure; node; j++, node = node->next) {
            expr = node->decl;
            wtgt->subrule[j].expr = expr;
            switch(expr->type) {
            case XC_EXPR_METHOD:
                minst = expr->minst;
                wtgt->subrule[j].type = BNH_SUBRULE_MTH;
                if(minst->type != XC_MINST_RES) {
                    fprintf(stderr, "ERROR: Unimplemented method type.\n");
                    break;
                }
                for(k = 0, found = 0; k < bnh->comp_count; k++) {
                    if(strcmp(bnh->comps[k].name, minst->comp->name) == 0) {
                        found = 1;
                        wtgt->subrule[j].comp_id = k;
                        break;
                    }
                }
                if(!found) {
                    fprintf(stderr, "ERROR: unknown component for method.\n");
                }
                if(wtgt->subrule[j].comp_id == bnh->comp_id) {
                    for(k = 0, found = 0; k < bnh->mth_count; k++) {
                        if(strcmp(minst->method->name, bnh->mths[k].name) ==
                           0) {
                            found = 1;
                            wtgt->subrule[j].mth_id = k;
                            break;
                        }
                    }
                    for(pnode = minst->params; pnode; pnode = pnode->next) {
                        if(pnode->type != XC_NODE_VMAP) {
                            fprintf(stderr,
                                    "ERROR: parsed parameter node is "
                                    "type %i, expected %i.\n",
                                    pnode->type, XC_NODE_VMAP);
                        }
                        vmap = pnode->decl;
                        if(strcmp(vmap->param, "in") == 0) {
                            wtgt->subrule[j].num_invar = xc_obj_len(vmap->vals);
                            wtgt->subrule[j].invars =
                                malloc(sizeof(*wtgt->subrule[j].invars) *
                                       wtgt->subrule[j].num_invar);
                            for(k = 0, vvnode = vmap->vals; vvnode;
                                k++, vvnode = vvnode->next) {
                                vv = vvnode->decl;
                                get_ifvar(bnh, vv->var_name, bnh->comp_id,
                                          &wtgt->subrule[j].invars[k].var_id);
                                wtgt->subrule[j].invars[k].ver = vv->ver;
                            }
                        } else if(strcmp(vmap->param, "out") == 0) {
                            wtgt->subrule[j].num_outvar =
                                xc_obj_len(vmap->vals);
                            wtgt->subrule[j].outvars =
                                malloc(sizeof(*wtgt->subrule[j].outvars) *
                                       wtgt->subrule[j].num_outvar);
                            for(k = 0, vvnode = vmap->vals; vvnode;
                                k++, vvnode = vvnode->next) {
                                vv = vvnode->decl;
                                get_ifvar(bnh, vv->var_name, bnh->comp_id,
                                          &wtgt->subrule[j].outvars[k].var_id);
                                wtgt->subrule[j].outvars[k].ver = vv->ver;
                            }
                        } else {
                            fprintf(stderr,
                                    "WARNING: unknown parameter '%s'"
                                    " loading subrule.\n",
                                    vmap->param);
                        }
                    }
                } else {
                    wtgt->subrule[j].mth_id = -1;
                }
                if(!found) {
                    fprintf(stderr, "ERROR: unknown method name.\n");
                }
                break;
            case XC_EXPR_ASG:
                wtgt->subrule[j].type = BNH_SUBRULE_ASG;
                wtgt->subrule[j].comp_id = bnh->comp_id;
                break;
            case XC_EXPR_XFR:
                wtgt->subrule[j].type = BNH_SUBRULE_PUB;
                minst = expr->src;
                arg_node = expr->src->args->decl;
                comp_name = (char *)arg_node->decl;
                if(minst->type != XC_MINST_G) {
                    fprintf(stderr,
                            "ERROR: unimplemented source transformation.\n");
                    break;
                }
                wtgt->subrule[j].comp_id = find_comp_idx_by_name(bnh, comp_name);
                if(wtgt->subrule[j].comp_id == -1) {
                    fprintf(stderr, "ERROR: unknown component for source.\n");
                }
                var_str = (char *)arg_node->next->decl;
                get_ifvar(bnh, var_str, wtgt->subrule[j].comp_id,
                          &wtgt->subrule[j].var_id);
                j++;
                wtgt->subrule[j].type = BNH_SUBRULE_SUB;
                minst = expr->tgt;
                arg_node = expr->tgt->args->decl;
                comp_name = (char *)arg_node->decl;
                if(minst->type != XC_MINST_G) {
                    fprintf(stderr,
                            "ERROR: unimplemented source transformation.\n");
                    break;
                }
                for(k = 0, found = 0; k < bnh->comp_count; k++) {
                    if(strcmp(bnh->comps[k].name, comp_name) == 0) {
                        found = 1;
                        wtgt->subrule[j].comp_id = k;
                        break;
                    }
                }
                if(!found) {
                    fprintf(stderr, "ERROR: unknown component for target.\n");
                }
                var_str = (char *)arg_node->next->decl;
                get_ifvar(bnh, var_str, wtgt->subrule[j].comp_id,
                          &wtgt->subrule[j].var_id);
                break;
            case XC_EXPR_MXFR:
                wtgt->subrule[j].type = BNH_SUBRULE_PUB;
                fus = expr->msrc;
                comp_name = fus->first->decl;
                wtgt->subrule[j].comp_id = find_comp_idx_by_name(bnh, comp_name); 
                if(wtgt->subrule[j].comp_id == -1) {
                    fprintf(stderr, "ERROR: unknown component '%s' for first source.\n", comp_name);
                }
            case XC_EXPR_INJ:
                wtgt->subrule[j].type = BNH_SUBRULE_INJ;
                comp_name = expr->comp_name;
                wtgt->subrule[j].comp_id = find_comp_idx_by_name(bnh, comp_name);
                if(wtgt->subrule[j].comp_id == -1) {
                    fprintf(stderr, "ERROR: unknown component '%s' for injection.\n", comp_name);
                }
                wtgt->subrule[j].inj_id = expr->inj_id;
            }
        }
    }

    bnh->known_objs = xc_new_ihash_map(tgt_count, 1);

    free(tgtnodes);

    return(0);
}

static void benesh_init_mpi(struct benesh_handle *bnh, MPI_Comm gcomm, int dummy)
{
    MPI_Comm_dup(gcomm, &bnh->gcomm);
    MPI_Comm_rank(gcomm, &bnh->grank);
    MPI_Comm_split(gcomm, dummy, bnh->grank, &bnh->mycomm);
    DEBUG_OUT("did split\n");
    MPI_Comm_rank(bnh->mycomm, &bnh->rank);
    MPI_Comm_size(bnh->mycomm, &bnh->comm_size);
    DEBUG_OUT("I have global rank %i and subgroup rank %i\n", bnh->grank, bnh->rank);
    bnh->root_rank = bnh->root_drank = -1;
    if(bnh->rank == 0) {
        if(dummy) {
            bnh->root_drank = bnh->grank;
        } else {
            bnh->root_rank = bnh->grank;
        }
    }
    DEBUG_OUT("doing reductions to find roots\n");
    MPI_Allreduce(MPI_IN_PLACE, &bnh->root_drank, 1, MPI_INT, MPI_MAX, bnh->gcomm);
    MPI_Allreduce(MPI_IN_PLACE, &bnh->root_rank, 1, MPI_INT, MPI_MAX, bnh->gcomm);

    DEBUG_OUT("Rank %i is root of the dummies, rank %i is normal root.\n", bnh->root_drank, bnh->root_rank);
    
}

int benesh_init(const char *name, const char *conf, MPI_Comm gcomm, int dummy, int wait,
                struct benesh_handle **handle)
{
    struct benesh_handle *bnh = calloc(1, sizeof(*bnh));
    struct tpoint_rule *rules;
    const char *envdebug = getenv("BENESH_DEBUG");
    const char *envna = getenv("BENESH_NA");
    char *na;
    struct hg_init_info hii = {0};
    char margo_conf[1024];
    struct margo_init_info mii = {0};
    int i;

    *handle = bnh;

    if(envdebug) {
        bnh->f_debug = 1;
    }

    bnh->name = strdup(name);
    bnh->dummy = dummy;

    if(envna) {
        DEBUG_OUT("using '%s' for NA string\n", envna);
        na = strdup(envna);
    } else {
        DEBUG_OUT("using default NA string (\"sockets\")\n");
        na = strdup("sockets");
    }

    APEX_FUNC_TIMER_START(benesh_init);
    benesh_init_mpi(bnh, gcomm, dummy);

    APEX_NAME_TIMER_START(1, "margo init");
    DEBUG_OUT("initializing margo...\n");

    sprintf(margo_conf, "{ \"use_progress_thread\" : true, \"rpc_thread_count\" : 1, \"progress_timeout_ub_msec\": 50}");
    hii.request_post_init = 1024;
    hii.auto_sm = 0;
    mii.hg_init_info = &hii;
    mii.json_config = margo_conf;

    bnh->mid = margo_init_ext(na, MARGO_SERVER_MODE, &mii);
    if(bnh->f_debug) {
        margo_set_log_level(bnh->mid, MARGO_LOG_TRACE);
    }
    APEX_TIMER_STOP(1);

    APEX_NAME_TIMER_START(2, "dspaces init");
    DEBUG_OUT("initializing dataspaces...\n");
    //dspaces_init_mpi(bnh->mycomm, &bnh->dsp);
    APEX_TIMER_STOP(2);
    APEX_NAME_TIMER_START(3, "ekt init");
    if(!bnh->dummy) {
        DEBUG_OUT("initializing EKT...\n");
        ekt_init(&bnh->ekth, name, bnh->mycomm, bnh->mid);
        ekt_register(bnh->ekth, BENESH_EKT_WORK, serialize_work, deserialize_work,
                 bnh, &bnh->work_type);
        ekt_watch(bnh->ekth, bnh->work_type, work_watch);

        ekt_register(bnh->ekth, BENESH_EKT_FINI, serialize_fini, deserialize_fini,
                 bnh, &bnh->fini_type);
        ekt_watch(bnh->ekth, bnh->fini_type, fini_watch);
        ekt_register(bnh->ekth, BENESH_EKT_TP, serialize_tpoint, deserialize_tpoint,
                 bnh, &bnh->tp_type);
        ekt_watch(bnh->ekth, bnh->tp_type, tpoint_watch);
    }
    APEX_TIMER_STOP(3);

    DEBUG_OUT("initializing mutexes...\n");
    ABT_mutex_create(&bnh->work_mutex);
    ABT_cond_create(&bnh->work_cond);
    ABT_mutex_create(&bnh->db_mutex);
    ABT_mutex_create(&bnh->data_mutex);
    ABT_cond_create(&bnh->data_cond);

    benesh_load_config(bnh, conf);
    APEX_NAME_TIMER_START(4, "benesh loadd");
    DEBUG_OUT("initializing workflow variables...\n");
    benesh_init_vars(bnh);
    DEBUG_OUT("initializing workflow domain...\n");
    benesh_init_doms(bnh);
    DEBUG_OUT("initializing workflow components...\n");
    benesh_init_comps(bnh);
    DEBUG_OUT("initializing workflow targets...\n");
    benesh_load_targets(bnh);
    DEBUG_OUT("initializing workflow touchpoints...\n");
    rules = get_tpoint_rules(bnh);

    bnsh_tpoint_init(bnh, rules, &bnh->tph);
    APEX_TIMER_STOP(4);

    if(!bnh->dummy) {
        ekt_enable(bnh->ekth);
    }

    if(wait) {
        if(!bnh->dummy && bnh->rank == 0) {
            DEBUG_OUT("waiting for bidirectional communication with other "
                      "components.\n");
            for(i = 0; i < bnh->comp_count; i++) {
                if(strcmp(bnh->comps[i].app, bnh->name) != 0) {
                    DEBUG_OUT("checking bidi status of component '%s'\n",
                              bnh->comps[i].app);
                    ekt_is_bidi(bnh->ekth, bnh->comps[i].app, 1);
                }
            }
        }
        MPI_Barrier(bnh->gcomm);
    } else {
        DEBUG_OUT("proceeding without waiting for other components.\n");
    }

    bnh->ready = 1;
    DEBUG_OUT("ready for workflow processing.\n");

    APEX_TIMER_STOP(0);

    return(0);
}

void print_work_node(FILE *stream, struct benesh_handle *bnh,
                     struct work_node *wnode)
{
    switch(wnode->type) {
    case BNH_WORK_OBJ:
        fprintf(stream, "%s of tgt %li",
                ((wnode->subrule == 0) ? "realization" : "initialization"),
                wnode->tgt - bnh->tgts);
        break;
    case BNH_WORK_RULE:
        fprintf(stream, "execution of tgt %li, rule %i", wnode->tgt - bnh->tgts,
                wnode->subrule);
        break;
    case BNH_WORK_CHAIN:
        fprintf(stream, "running chain in tgt %li",
                (wnode->link ? (wnode->link->tgt - bnh->tgts) : -1));
        break;
    default:
        fprintf(stream, "ERROR! Unknown work type in print_work_node");
    }
}

void print_work_node_nl(FILE *stream, struct benesh_handle *bnh,
                        struct work_node *wnode)
{
    switch(wnode->type) {
    case BNH_WORK_OBJ:
        fprintf(stream, "%s of tgt %li",
                ((wnode->subrule == 0) ? "realization" : "initialization"),
                wnode->tgt - bnh->tgts);
        break;
    case BNH_WORK_RULE:
        fprintf(stream, "execution of tgt %li, rule %i", wnode->tgt - bnh->tgts,
                wnode->subrule);
        break;
    case BNH_WORK_CHAIN:
        fprintf(stream, "running chain in tgt %li",
                (wnode->link ? (wnode->link->tgt - bnh->tgts) : -1));
        break;
    default:
        fprintf(stream, "ERROR! Unknown work type in print_work_node");
    }
    fprintf(stream, "\n");
}

struct work_node *deque_work(struct benesh_handle *bnh)
{
    struct work_node *wnode = bnh->wqueue_tail;

    if(!wnode) {
        return NULL;
    }

    if(bnh->wqueue_tail == bnh->wqueue_head) {
        bnh->wqueue_tail = bnh->wqueue_head = NULL;
        wnode->next = wnode->prev = NULL;
        return wnode;
    }

    bnh->wqueue_tail = wnode->prev;
    bnh->wqueue_tail->next = NULL;
    wnode->prev = NULL;
    return (wnode);
}

struct wf_var *get_gvar(struct benesh_handle *bnh, const char *name)
{
    int i;

    for(i = 0; i < bnh->gvar_count; i++) {
        if(strcmp(bnh->gvars[i].name, name) == 0) {
            return (&bnh->gvars[i]);
        }
    }

    fprintf(stderr, "WARNING: unknown var %s\n", name);

    return (NULL);
}

struct wf_var *get_ifvar(struct benesh_handle *bnh, const char *name,
                         int comp_id, int *var_id)
{
    int i;

    for(i = 0; i < bnh->ifvar_count; i++) {
        if(strcmp(bnh->ifvars[i].name, name) == 0 &&
           bnh->ifvars[i].comp_id == comp_id) {
            if(var_id) {
                *var_id = i;
            }
            return (&bnh->ifvars[i]);
        }
    }

    fprintf(stderr, "WARNING: unknown var %s\n", name);
    if(var_id) {
        *var_id = -1;
    }
    return (NULL);
}

double eval_expr(struct benesh_handle *bnh, struct xc_pqexpr *pqx)
{
    struct wf_var *var;

    switch(pqx->type) {
    case XC_PQ_INT:
        return ((double)(*(int *)pqx->val));
    case XC_PQ_REAL:
        return (*(double *)pqx->val);
    case XC_PQ_ADD:
        return (eval_expr(bnh, pqx->lside) + eval_expr(bnh, pqx->rside));
    case XC_PQ_SUB:
        return (eval_expr(bnh, pqx->lside) - eval_expr(bnh, pqx->rside));
    case XC_PQ_VAR:
        var = get_gvar(bnh, pqx->val);
        return (var->val);
    default:
        fprintf(stderr, "ERROR: unknown expression type during evaluation.\n");
        return 0.0;
    }
}

void handle_asg(struct benesh_handle *bnh, struct sub_rule *subrule)
{
    struct xc_minst *lhs;
    struct xc_pqexpr *rhs;
    struct xc_list_node *node;
    struct wf_var *var;
    int found;
    int i;

    lhs = subrule->expr->lhs;
    node = lhs->args->decl;
    if(node->next || node->type != XC_NODE_STR) {
        fprintf(stderr, "ERROR: Unsupported lhs when doing assignement.\n");
    }

    var = get_gvar(bnh, node->decl);

    rhs = subrule->expr->rhs;
    var->val = eval_expr(bnh, rhs);

    printf("new value of %s is %f\n", var->name, var->val);
}

void handle_method(struct benesh_handle *bnh, struct sub_rule *subrule)
{
    struct wf_method *mth;
    struct xc_expr *expr;

    mth = &bnh->mths[subrule->mth_id];
    if(!mth->method) {
        //fprintf(stderr, "ERROR: no mapped function for method '%s'.\n",
        //        subrule->expr->minst->method->name);
    } else {
        mth->method(bnh, mth->arg);
    }
}

// Publishing once per transfer rule - need to collapse redundant.
void publish_var(struct benesh_handle *bnh, struct wf_var *var,
                 struct wf_target *tgt, int subrule, int64_t *var_maps,
                 double *ol_off_lb, double *ol_off_ub)
{
    struct wf_component *comp = &bnh->comps[bnh->comp_id];
    struct wf_domain *dom = var->dom;
    uint64_t *lb, *ub;
    char ds_var_name[100];
    char num_str[68];
    double pitch;
    int tgt_id;
    int i;

    APEX_FUNC_TIMER_START(publish_var);
    sprintf(ds_var_name, "%s.%s", comp->name, var->name);
    tgt_id = tgt - bnh->tgts;
    sprintf(num_str, ".%i", tgt_id);
    strcat(ds_var_name, num_str);
    sprintf(num_str, ".%i", subrule);
    strcat(ds_var_name, num_str);
    for(i = 0; i < tgt->num_vars; i++) {
        sprintf(num_str, ".%" PRId64, var_maps[i]);
        strcat(ds_var_name, num_str);
    }

    DEBUG_OUT("publishing %s along domain %s\n", ds_var_name, dom->full_name);

    lb = malloc(sizeof(*lb) * dom->dim);
    ub = malloc(sizeof(*ub) * dom->dim);
    for(i = 0; i < dom->dim; i++) {
        pitch = dom->l_grid_dims[i] / dom->l_grid_pts[i];
        if(dom->l_offset[i] > ol_off_lb[i]) {
            lb[i] = (dom->l_offset[i] - ol_off_lb[i]) / pitch;
        } else {
            lb[i] = 0;
        }
        ub[i] = lb[i] + dom->l_grid_pts[i] - 1;
        DEBUG_OUT(
            "geometry for dimension %i: grid_dims = %lf, grid_pts = %" PRIu64
            ", pitch = %lf, offset = %lf, overlap_offset = %lf, lb = %" PRIu64
            ", ub = %" PRIu64 "\n",
            i, dom->l_grid_dims[i], dom->l_grid_pts[i], pitch, dom->l_offset[i],
            ol_off_lb[i], lb[i], ub[i]);
    }

// TODO: more sophisticated type handling, version control
#ifdef BDEBUG
    fprintf(stderr, "dpaces_put / lb[0] = %" PRIu64 ", ub[0] = %" PRIu64 "\n",
            lb[0], ub[0]);
#endif /* BDEBUG */
    dspaces_put(bnh->dsp, ds_var_name, 0, sizeof(double), dom->dim, lb, ub,
                var->buf);
    free(lb);
    free(ub);
    APEX_TIMER_STOP(0);
}

struct data_sub {
    struct benesh_handle *bnh;
    struct wf_target *tgt;
    int subrule;
    int64_t *var_maps;
    struct wf_var *var;
    uint64_t *lb;
    uint64_t *ub;
    double lint, uint;
    int waiting;
};

// TODO: cache first part...
void matrix_copy(void *tgt_buf, int ndim, uint64_t *tlb, uint64_t *tub,
                 void *src_buf, uint64_t *slb, uint64_t *sub, int elem_size)
{
    int fastest_disp_dim = -1;
    int copy_len = elem_size;
    int ncopy = 1;
    size_t tgt_size, src_size, offset, term, stride;
    int i;

    tgt_size = src_size = 1;
    offset = 0;
    for(i = ndim - 1; i >= 0; i--) {
        if(fastest_disp_dim == -1) {
            copy_len *= (sub[i] - slb[i]) + 1;
            if(tlb[i] != slb[i] || tub[i] != sub[i]) {
                fastest_disp_dim = i;
            }
        } else {
            ncopy *= (sub[i] - slb[i]) + 1;
        }
        offset += (slb[i] - tlb[i]) * tgt_size;
        term += (sub[i] - tlb[i]) * tgt_size;
        tgt_size *= (tub[i] - tlb[i]) + 1;
        src_size *= (sub[i] - slb[i]) + 1;
    }

    if(fastest_disp_dim == -1) {
        memcpy(tgt_buf, src_buf, (size_t)elem_size * tgt_size);
        return;
    }

    stride = elem_size * (term - offset) / (size_t)ncopy;
    tgt_buf += offset * elem_size;
    for(i = 0; i < ncopy; i++) {
        memcpy(tgt_buf, src_buf, copy_len);
        tgt_buf += stride;
        src_buf += copy_len;
    }
}

void matrix_copy_interp(void *tgt_buf, int ndim, uint64_t *tlb, uint64_t *tub,
                        void *src_buf, uint64_t *slb, uint64_t *sub,
                        int elem_size, double lint, double uint)
{
    int nline, line_size;
    size_t src_size, tgt_size, offset, term, stride;
    double pitch;
    int i, j;

    offset = term = 0;
    tgt_size = src_size = 1;
    for(i = ndim - 1; i >= 0; i--) {
        offset += (slb[i] - tlb[i]) * tgt_size;
        term += (sub[i] - tlb[i]) * tgt_size;
        tgt_size *= (tub[i] - tlb[i]) + 1;
        src_size *= (sub[i] - slb[i]) + 1;
    }

    line_size = ((sub[ndim - 1] - slb[ndim - 1]) + 1);
    nline = src_size / line_size;
    stride = elem_size * ((term - offset) + 1) / (size_t)nline;
    pitch = (uint - lint) / (line_size - 1);
    tgt_buf += offset * elem_size;
    for(i = 0; i < nline; i++) {
        for(j = 0; j < line_size; j++) {
            ((double *)tgt_buf)[j] =
                (1 - (lint + (j * pitch))) * ((double *)tgt_buf)[j] +
                (lint + (j * pitch)) * ((double *)src_buf)[j];
        }
        tgt_buf += stride;
        src_buf += elem_size * line_size;
    }
}

int handle_notify(dspaces_client_t dsp, struct dspaces_req *req,
                  void *data_sub_v)
{
    struct data_sub *ds = (struct data_sub *)data_sub_v;
    struct benesh_handle *bnh = ds->bnh;
    struct wf_var *var = ds->var;
    struct wf_domain *dom = var->dom;
    struct work_node wnode = {0};
    uint64_t *llb, *lub;
    double pitch;
    struct obj_entry *ent;
    int i;

    APEX_FUNC_TIMER_START(handle_notify);
    DEBUG_OUT("received notification for target %li, subrule %i.\n",
              ds->tgt - bnh->tgts, ds->subrule);

    while(!bnh->ready) {
        // Change to wait condition
        sleep(1);
    }

    llb = malloc(sizeof(*llb) * dom->dim);
    lub = malloc(sizeof(*lub) * dom->dim);
    for(i = 0; i < dom->dim; i++) {
        pitch = dom->l_grid_dims[i] / dom->l_grid_pts[i];
        llb[i] = (uint64_t)(dom->l_offset[i] / pitch);
        lub[i] = llb[i] + dom->l_grid_pts[i] - 1;
    }

    DEBUG_OUT("checking whether we are blocking on these data yet.\n");
    ABT_mutex_lock(bnh->data_mutex);
    while(!ds->waiting) {
        DEBUG_OUT("not blocking yet. Waiting...\n");
        ABT_cond_wait(bnh->data_cond, bnh->data_mutex);
    }
    ABT_mutex_unlock(bnh->data_mutex);

    DEBUG_OUT("copying data in local buffers\n");
    // matrix_copy(var->buf, dom->dim, llb, lub, req->buf, ds->lb, ds->ub,
    //            sizeof(double));
    matrix_copy_interp(var->buf, dom->dim, llb, lub, req->buf, ds->lb, ds->ub,
                       sizeof(double), ds->lint, ds->uint);

    wnode.tgt = ds->tgt;
    wnode.subrule = ds->subrule - 1; // the sub
    wnode.var_maps = ds->var_maps;

    DEBUG_OUT("activating work object\n");
    ABT_mutex_lock(bnh->db_mutex);

    ent = get_object_entry(bnh, wnode.tgt, wnode.subrule, wnode.var_maps, 1);
    if(!ent) {
        fprintf(stderr, "ERROR: null entry when handling subscription "
                        "(shouldn't happen).\n");
    }
    if(ent->realized) {
        /*
         * TODO: this warning is being triggered because the publisher sends an
         * announcement of the publish, which is needed by any rank that doesn't
         * subscribe to that publish, and hence can't directly see that it has
         * happened. This may be necessary some or even most of the time, but
         * there are workflows where, for example, the publish is the only
         * 'foreign' work item in the target rule, and so no extra EKT
         * announcement is needed.
         */
        // fprintf(stderr, "WARNING: already received data.\n");
    }
    ent->realized = 1;
    ABT_mutex_unlock(bnh->db_mutex);
    activate_subs(bnh, &wnode);

    free(ds);
    APEX_TIMER_STOP(0);

    return (0);
}

static void sub_var(struct benesh_handle *bnh, struct work_node *wnode,
                    struct wf_var *src_var, struct wf_var *dst_var,
                    struct wf_target *tgt, int subrule, int64_t *var_maps,
                    double *lbf, double *ubf, double *ol_off_lb,
                    double *ol_off_ub)
{
    struct wf_domain *dom = src_var->dom;
    struct wf_domain *l_dom = dst_var->dom;
    struct data_sub *ds;
    struct wf_component *comp;
    char ds_var_name[100];
    char num_str[68];
    int tgt_id;
    uint64_t *lb, *ub;
    double pitch;
    double glb, gub;
    int i;

    APEX_FUNC_TIMER_START(sub_var);
    comp = &bnh->comps[src_var->comp_id];
    sprintf(ds_var_name, "%s.%s", comp->name, src_var->name);
    tgt_id = tgt - bnh->tgts;
    sprintf(num_str, ".%i", tgt_id);
    strcat(ds_var_name, num_str);
    sprintf(num_str, ".%i", subrule - 1); // the src subrule
    strcat(ds_var_name, num_str);
    for(i = 0; i < tgt->num_vars; i++) {
        sprintf(num_str, ".%" PRId64, var_maps[i]);
        strcat(ds_var_name, num_str);
    }

    ub = malloc(sizeof(*ub) * dom->dim);
    lb = malloc(sizeof(*lb) * dom->dim);

    for(i = 0; i < dom->dim; i++) {
        pitch = l_dom->l_grid_dims[i] / l_dom->l_grid_pts[i];
        lb[i] = ((lbf[i] - dom->lb[i]) - ol_off_lb[i]) / pitch;
        ub[i] = ((ubf[i] - dom->lb[i]) - ol_off_lb[i]) / pitch - 1;
    }

    glb = dom->lb[0] < l_dom->lb[0] ? l_dom->lb[0] : dom->lb[0];
    gub = dom->ub[0] < l_dom->ub[0] ? dom->ub[0] : l_dom->ub[0];

    ds = malloc(sizeof(*ds));
    ds->bnh = bnh;
    ds->tgt = tgt;
    ds->subrule = subrule;
    ds->var_maps = var_maps;
    ds->var = dst_var;
    ds->ub = malloc(sizeof(*ds->ub) * dom->dim);
    ds->lb = malloc(sizeof(*ds->lb) * dom->dim);
    for(i = 0; i < dom->dim; i++) {
        pitch = l_dom->l_grid_dims[i] / l_dom->l_grid_pts[i];
        ds->lb[i] = (lbf[i] - l_dom->lb[i]) / pitch;
        ds->ub[i] = (ubf[i] - l_dom->lb[i]) / pitch - 1;
    }
    ds->lint = (lbf[0] - glb) / (gub - glb);
    ds->uint = (ubf[0] - glb) / (gub - glb);
    ds->waiting = 0;

    wnode->ds = ds;
    DEBUG_OUT("subscribing to %s\n", ds_var_name);
    wnode->req = dspaces_sub(bnh->dsp, ds_var_name, 0, sizeof(double), dom->dim,
                             lb, ub, handle_notify, ds);
    APEX_TIMER_STOP(0);
}

// maybe cache these results?
int local_overlap(struct wf_domain *loc_dom, struct wf_domain *glob_dom,
                  double **lb_overlap, double **ub_overlap)
{
    double llb, lub, glb, gub;
    int i;

    if(loc_dom->dim != glob_dom->dim) {
        fprintf(stderr, "INFO: dimensional mismatch in overlap check.\n");
        return 0;
    }

    if(lb_overlap && ub_overlap) {
        *lb_overlap = malloc(sizeof(*lb_overlap) * loc_dom->dim);
        *ub_overlap = malloc(sizeof(*ub_overlap) * loc_dom->dim);
    }

    for(i = 0; i < loc_dom->dim; i++) {
        llb = loc_dom->lb[i] + loc_dom->l_offset[i];
        lub = llb + loc_dom->l_grid_dims[i];
        glb = glob_dom->lb[i];
        gub = glob_dom->ub[i];
        if(llb >= gub || lub <= glb) {
            if(lb_overlap && ub_overlap) {
                free(*lb_overlap);
                free(*ub_overlap);
            }
            return (0);
        }
        if(lb_overlap && ub_overlap) {
            (*lb_overlap)[i] = (llb < glb) ? glb : llb;
            (*ub_overlap)[i] = (lub < gub) ? lub : gub;
        }
    }

#ifdef BDEBUG
    if(lb_overlap && ub_overlap) {
        for(i = 0; i < loc_dom->dim; i++) {
            fprintf(stderr, "overlap on dim %i is %lf to %lf\n", i,
                    (*lb_overlap)[i], (*ub_overlap)[i]);
        }
    }
#endif /* BDEBUG */

    return (1);
}

void overlap_offset(struct wf_domain *loc_dom, struct wf_domain *glob_dom,
                    double **lb_offset, double **ub_offset)
{
    int i;

    if(loc_dom->dim != glob_dom->dim) {
        return;
    }

    *lb_offset = malloc(sizeof(**lb_offset) * loc_dom->dim);
    *ub_offset = malloc(sizeof(**ub_offset) * glob_dom->dim);

    for(i = 0; i < loc_dom->dim; i++) {
        if(loc_dom->ub[i] < glob_dom->lb[i] ||
           loc_dom->lb[i] > glob_dom->ub[i]) {
            fprintf(stderr, "WARNING: no global overlap when trying to do data "
                            "exchange.\n");
            *lb_offset = *ub_offset = NULL;
            return;
        }
        (*lb_offset)[i] = loc_dom->lb[i] > glob_dom->lb[i]
                              ? 0
                              : glob_dom->lb[i] - loc_dom->lb[i];
        (*ub_offset)[i] =
            loc_dom->ub[i] < glob_dom->ub[i] ? loc_dom->ub[i] : glob_dom->ub[i];
        (*ub_offset)[i] -= loc_dom->lb[i];
    }
}

#define BNH_FIELD_SEND 1
#define BNH_FIELD_RECV 2
#define BNH_TERM -1

static void command_dummies(struct benesh_handle *bnh, int command, int comp_id, int var_id)
{
    int cmd[3] = {command, comp_id, var_id};

    MPI_Send(cmd, 3, MPI_INT, bnh->root_drank, 0, bnh->gcomm);
}

int handle_pub(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct wf_target *tgt = wnode->tgt;
    struct sub_rule *prule = &tgt->subrule[wnode->subrule - 1];
    struct sub_rule *srule = &tgt->subrule[wnode->subrule];
    struct wf_var *src_var = &bnh->ifvars[prule->var_id];
    struct wf_var *dst_var = &bnh->ifvars[srule->var_id];
    struct wf_component *dst_comp = &bnh->comps[srule->comp_id];
    struct wf_domain *src_dom = src_var->dom;
    struct wf_domain *dst_dom = dst_var->dom;
    struct field_handle *field;
    struct app_hndl *apph;
    double *goff_lb, *goff_ub;
    int signal, smin, smax;
    int i;

    DEBUG_OUT("publishing variable %s to %s\n", src_var->name, dst_comp->name);

    if(!src_dom) {
        fprintf(stderr, "ERROR: source domain is empty\n");
    }

    if(src_dom->comm_type == BNH_COMM_RDV_SRV ||
            src_dom->comm_type == BNH_COMM_RDV_CLI) {
        // TODO this is not the right way to do this
        if(src_dom->comm_type == BNH_COMM_RDV_CLI) {
            signal = 1;
            apph = bnh->comps[bnh->comp_id].cpl_apph;
        } else {
            signal = (2 * srule->comp_id) + 1;
            apph = dst_comp->cpl_apph;
        }
        do {
            signal_minmax(bnh, signal, &smin, &smax);
            if(smin == 0) {
                DEBUG_OUT("bailing before starting a send to avoid deadlock!\n");
                return(3);
            } else if(smin < signal) {
                DEBUG_OUT("requeuing send to prevent deadlock\n");
                return(0);
            }
        } while(smax > signal);
        DEBUG_OUT("doing a field transfer (app %p)\n", (void *)apph);

        if(bnh->rank == 0 && bnh->root_drank > -1) {
            command_dummies(bnh, BNH_FIELD_SEND, srule->comp_id, prule->var_id);
        }

        dst_comp->send_phase_open = 0; //DEBUG!!!!
        if(dst_comp->send_phase_open == 0) {
            DEBUG_OUT("starting send phase\n");
            app_begin_send_phase(apph);
            dst_comp->send_phase_open = 1;
        }
       
        DEBUG_OUT("%i fields to send\n", src_var->num_fields); 
        for(i = 0; i < src_var->num_fields; i++) {
            DEBUG_OUT("sending %s, field %i on %s using %p\n", src_var->name, i, src_dom->full_name, field);
            field = src_var->fields[i];
            cpl_send_field(field);
            DEBUG_OUT("sent\n"); 
        }
        app_end_send_phase(apph); // DEBUG!!!!
    } else if(local_overlap(src_dom, dst_dom, NULL, NULL)) {
        overlap_offset(src_dom, dst_dom, &goff_lb, &goff_ub);
        publish_var(bnh, src_var, tgt, wnode->subrule, wnode->var_maps, goff_lb,
                    goff_ub);
    }

    return(1);
}

int handle_sub(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct wf_target *tgt = wnode->tgt;
    struct sub_rule *prule =
        &tgt->subrule[wnode->subrule - 2]; // subrules start at 1
    struct sub_rule *srule = &tgt->subrule[wnode->subrule - 1];
    struct wf_var *src_var = &bnh->ifvars[prule->var_id];
    struct wf_var *dst_var = &bnh->ifvars[srule->var_id];
    struct wf_domain *src_dom = src_var->dom;
    struct wf_domain *dst_dom = dst_var->dom;
    double *lb, *ub, *goff_lb, *goff_ub;

    if(dst_dom->comm_type == BNH_COMM_RDV_SRV ||
       dst_dom->comm_type == BNH_COMM_RDV_CLI) {
        wnode->sub_req = 1;
        return 1;
    }

    if(local_overlap(dst_dom, src_dom, &lb, &ub)) {
        overlap_offset(src_dom, dst_dom, &goff_lb, &goff_ub);
        sub_var(bnh, wnode, src_var, dst_var, tgt, wnode->subrule,
                wnode->var_maps, lb, ub, goff_lb, goff_ub);
        wnode->sub_req = 1;
        return 1;
    } else {
        wnode->sub_req = 0;
        return 0;
    }
}

int get_with_redev(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct wf_target *tgt = wnode->tgt;
    struct sub_rule *prule =
        &wnode->tgt->subrule[wnode->subrule - 2]; // subrules start at 1
    struct wf_component *src_comp = &bnh->comps[prule->comp_id];
    struct sub_rule *srule = &tgt->subrule[wnode->subrule - 1];
    struct wf_component *dst_comp = &bnh->comps[srule->comp_id];
    struct wf_var *dst_var = &bnh->ifvars[srule->var_id];
    struct wf_var *src_var = &bnh->ifvars[prule->var_id];
    struct wf_domain *src_dom = src_var->dom;
    struct wf_domain *dst_dom = dst_var->dom;
    struct app_hndl *apph;
    struct field_handle *field;
    size_t num_elem;
    int signal, smin, smax;
    int i;

    DEBUG_OUT("getting %s from %s\n", dst_var->name, src_comp->name);
    if(dst_dom->comm_type == BNH_COMM_RDV_CLI) {
        signal = 2;
        apph = bnh->comps[bnh->comp_id].cpl_apph;
    } else {
        signal = 2 * (prule->comp_id + 1);
        apph = src_comp->cpl_apph;
    }
    do {
        signal_minmax(bnh, signal, &smin, &smax);
        if(smin == 0) {
            // some other rank has left the work handling loop.
            // if they enter a Barrier before returning, we'll
            // be deadlocked. Better bail.
            DEBUG_OUT("bailing before starting a recv to avoid deadlock!\n");
            return(3);
        } else if(smin < signal) {
            /* different ranks are trying to start different
             * communication phases. All but the lowest id
             * should requeue
             */
            DEBUG_OUT("requeuing recv to avoid deadlock.\n");
            return(0);  
        } 
    } while(smax > signal);

    if(bnh->rank == 0 && bnh->root_drank > -1) {
        command_dummies(bnh, BNH_FIELD_RECV, prule->comp_id, srule->var_id);
    }

    src_comp->recv_phase_open = 0; //DEBUG!!!
    if(src_comp->recv_phase_open == 0) {
        DEBUG_OUT("Starting receive phase (apph %p)\n", (void *)apph);
        app_begin_recv_phase(apph);
        src_comp->recv_phase_open = 1;
    }
    DEBUG_OUT("%i fields to get.\n", dst_var->num_fields);
    for(i = 0; i < dst_var->num_fields; i++) {
        field = dst_var->fields[i];
        DEBUG_OUT("receiving %s, field %i\n", dst_var->name, i);
        cpl_recv_field(field);
    }
    app_end_recv_phase(apph);

    return (1);
}

int check_sub(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct wf_target *tgt = wnode->tgt;
    struct sub_rule *srule;
    struct wf_var *dst_var;
    struct wf_domain *dom;
    struct data_sub *ds = wnode->ds;
    int ret;
    int result;

    APEX_FUNC_TIMER_START(check_sub);
    if(wnode->sub_req) {
        srule = &tgt->subrule[wnode->subrule - 1];
        dst_var = &bnh->ifvars[srule->var_id];
        dom = dst_var->dom;
        if(dom->comm_type == BNH_COMM_RDV_SRV ||
           dom->comm_type == BNH_COMM_RDV_CLI) {
            return(get_with_redev(bnh, wnode));
        } else {
            APEX_NAME_TIMER_START(1, "data_lock_csa");
            ABT_mutex_lock(bnh->data_mutex);
            APEX_TIMER_STOP(1);
            ds->waiting = 1;
            ABT_cond_broadcast(bnh->data_cond);

            ABT_mutex_unlock(bnh->data_mutex);
            DEBUG_OUT("waiting for dspaces_check_sub\n");
            APEX_NAME_TIMER_START(2, "b_dspaces_check_sub");

            ret = dspaces_check_sub(bnh->dsp, wnode->req, 1, &result) ==
                  DSPACES_SUB_DONE;
            APEX_TIMER_STOP(2);
        }
        DEBUG_OUT("dspaces_check_sub finished\n");
        APEX_TIMER_STOP(0);
        return (ret);
    } else {
        APEX_TIMER_STOP(0);
        return (1);
    }
    APEX_TIMER_STOP(0);
}

void send_field_by_name(struct benesh_handle *bnh, const char *fname)
{
    struct field_handle *field;
    struct wf_var *var;
    int i;

    var = match_local_ifvar(bnh, fname);
    for(i = 0; i < var->num_fields; i++) {
        field = var->fields[i];
        cpl_send_field(field);
    }
}

void send_list(struct benesh_handle *bnh, struct wf_component *comp, char **list, int len)
{
    int i; 

    app_begin_send_phase(comp->cpl_apph);
    for(i = 0; i < len; i++) {
        send_field_by_name(bnh, list[i]);
    }
    app_end_send_phase(comp->cpl_apph);
}

void do_inject(struct benesh_handle *bnh, struct sub_rule *subrule)
{
    char *inj_id = subrule->inj_id;
    char id1 = inj_id[0];
    int id2 = atoi(&inj_id[1]);
    struct wf_var *var;
    struct wf_domain *dom;
    struct field_handle *field;
    struct wf_component *comp;
    int i, j;

    DEBUG_OUT("injection signature: %i, %i\n", id1, id2);
}

int handle_subrule(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct wf_target *tgt = wnode->tgt;
    int64_t *var_maps = wnode->var_maps;
    struct sub_rule *subrule = &tgt->subrule[wnode->subrule - 1];
    int result;

    DEBUG_OUT("handling a subrule\n");
    APEX_FUNC_TIMER_START(handle_subrule);
    switch(subrule->type) {
    case BNH_SUBRULE_ASG:
        DEBUG_OUT("subrule is an assignment\n");
        handle_asg(bnh, subrule);
        break;
    case BNH_SUBRULE_MTH:
        DEBUG_OUT("subrule is a method\n");
        handle_method(bnh, subrule);
        break;
    case BNH_SUBRULE_PUB:
        DEBUG_OUT("subrule is a publish event\n");
        return(handle_pub(bnh, wnode));
    case BNH_SUBRULE_SUB:
        DEBUG_OUT("subrule is a subscribe event\n");
        return(check_sub(bnh, wnode));
    case BNH_SUBRULE_INJ:
        DEBUG_OUT("subrule is an injection event\n");
        do_inject(bnh, subrule);
        break;
    default:
        fprintf(stderr, "ERROR: unknown subrule type.\n");
    }
    APEX_TIMER_STOP(0);
    return (1);
}

static int deps_met(struct benesh_handle *bnh, struct wf_target *tgt,
                    int64_t *map_vals)
{
    struct pq_obj **dep_tgts = NULL;
    struct wf_target *dep_rule;
    int64_t *dep_map_vals;
    int i, j;

    APEX_FUNC_TIMER_START(deps_met);
    if(!tgt->ndep) {
        return (1);
    }

    dep_tgts = malloc(sizeof(*dep_tgts) * tgt->ndep);
    for(i = 0; i < tgt->ndep; i++) {
        dep_tgts[i] = resolve_obj(bnh, tgt->deps[i], tgt->num_vars,
                                  tgt->tgt_vars, map_vals);
        dep_rule = find_target_rule(bnh, dep_tgts[i], &dep_map_vals);
        if(!object_realized(bnh, dep_rule, dep_map_vals)) {
            if(bnh->f_debug) {
                DEBUG_OUT("dep not met: ");
                print_pq_obj_nl(stderr, dep_tgts[i]);
                DEBUG_OUT(" dep rule id = %li\n", dep_rule - bnh->tgts);
                for(j = 0; j < dep_rule->num_vars; j++) {
                    DEBUG_OUT("   %s => %" PRIu64 "\n", dep_rule->tgt_vars[j],
                              dep_map_vals[j]);
                }
            }
            free(dep_map_vals);
            free(dep_tgts);
            APEX_TIMER_STOP(0);
            return 0;
        }
    }
    free(dep_tgts);
    APEX_TIMER_STOP(0);
    return 1;
}

static int handle_work(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct work_node *link;
    int result;

    APEX_FUNC_TIMER_START(handle_work);
    switch(wnode->type) {
    case BNH_WORK_OBJ:
        if(wnode->subrule == -1) {
            APEX_NAME_TIMER_START(1, "lock_work_hwa");
            ABT_mutex_lock(bnh->work_mutex);
            APEX_TIMER_STOP(1);
            if(deps_met(bnh, wnode->tgt, wnode->var_maps)) {
                ABT_mutex_unlock(bnh->work_mutex);
                DEBUG_OUT("Starting object tgt %li\n", wnode->tgt - bnh->tgts);
                break;
            }
            ABT_mutex_unlock(bnh->work_mutex);
            return 0;
        }
        if(wnode->subrule != 0) {
            fprintf(stderr, "WARNING: Object work handling confusion.\n");
        }
        break;
    case BNH_WORK_RULE:
        return (handle_subrule(bnh, wnode));
        break;
    case BNH_WORK_CHAIN:
        for(link = wnode->link; link->next; link = link->next) {
            result = handle_work(bnh, link);
            if(result == 0) {
                wnode->link = link; // memory leak
                return (0);
            } else if(result == 3) {
                wnode->link = link; // memory leak
                DEBUG_OUT("bailed out of subrule handling...\n");
                return(3);
            }
        }
        ABT_mutex_lock(bnh->work_mutex);
        benesh_make_active(bnh, link); // to catch announce
        ABT_mutex_unlock(bnh->work_mutex);
        break;
    default:
        fprintf(stderr, "ERROR: unknown work entry type.\n");
    }
    APEX_TIMER_STOP(0);

    return (1);
}

int activate_subs(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct obj_entry *ent;
    struct obj_sub_node *snode;
    struct work_node *sub;
    int activated = 0;

    DEBUG_OUT("activating subscribers to work item\n");

    APEX_FUNC_TIMER_START(activate_subs);
    if(bnh->f_debug) {
        switch(wnode->type) {
        case BNH_WORK_OBJ:
            DEBUG_OUT("activating subs of target %li %s\n",
                      wnode->tgt - bnh->tgts,
                      ((wnode->subrule == 0) ? "realization" : "initiation"));
            break;
        case BNH_WORK_RULE:
            DEBUG_OUT("activating subs of target %li, rule %i\n",
                      wnode->tgt - bnh->tgts, wnode->subrule);
            break;
        case BNH_WORK_CHAIN:
            DEBUG_OUT("activating subs of a chain in target %li\n",
                      wnode->tgt - bnh->tgts);
            break;
        case BNH_WORK_ANNOUNCE:
            DEBUG_OUT(
                "activating subs of received work with target %li, rule %i\n",
                wnode->tgt - bnh->tgts, wnode->subrule);
            break;
        default:
            DEBUG_OUT("ERROR: unknown work type in activate_subs\n");
        }
    }

    APEX_NAME_TIMER_START(1, "db_lock_asa");
    ABT_mutex_lock(bnh->db_mutex);
    APEX_TIMER_STOP(1);

    DEBUG_OUT("got db lock\n");

    ent = get_object_entry(bnh, wnode->tgt, wnode->subrule, wnode->var_maps, 0);

    if(ent) {
        DEBUG_OUT("updating object registry\n");
        for(snode = ent->subs; snode; snode = snode->next) {
            if(!snode->done) {
                sub = snode->sub;
                sub->deps--;
                if(sub->deps) {
                    continue;
                }
                snode->done = 1;
                ABT_mutex_unlock(bnh->db_mutex);
                APEX_NAME_TIMER_START(2, "work_lock_asa");
                ABT_mutex_lock(bnh->work_mutex);
                APEX_TIMER_STOP(2);
                benesh_make_active(bnh, sub);
                ABT_cond_signal(bnh->work_cond);
                ABT_mutex_unlock(bnh->work_mutex);
                APEX_NAME_TIMER_START(3, "db_lock_asb");
                ABT_mutex_lock(bnh->db_mutex);
                APEX_TIMER_STOP(3);
                activated++;
            }
        }
        DEBUG_OUT("registry updated\n");
    }
    ABT_mutex_unlock(bnh->db_mutex);
    APEX_TIMER_STOP(0);
    return (activated);
}

void announce_work(struct benesh_handle *bnh, struct work_node *wnode)
{
    struct work_announce announce;
    struct wf_target *tgt;
    int i;

    announce.comp_id = bnh->comp_id;
    announce.tgt_id = wnode->tgt - bnh->tgts;
    announce.tgt_vars = wnode->var_maps;
    announce.subrule_id = wnode->subrule;

    DEBUG_OUT("announcing rule %i, subrule %i\n", announce.tgt_id,
              announce.subrule_id);
    if(bnh->f_debug && wnode->subrule > 0) {
        tgt = wnode->tgt;
        for(i = 0; i < tgt->num_vars; i++) {
            DEBUG_OUT(" tgt_var %i = %" PRIu64 "\n", i, wnode->var_maps[i]);
        }
    }
    APEX_NAME_TIMER_START(1, "ekt_tell_work");
    ekt_tell(bnh->ekth, NULL, bnh->work_type, &announce);
    APEX_TIMER_STOP(1);
}

static void benesh_end_phases(struct benesh_handle *bnh)
{
    struct wf_component *comp;
    int i;

    for(i = 0; i < bnh->comp_count; i++) {
        comp = &bnh->comps[i];
        if(comp->recv_phase_open) {
            app_end_recv_phase(comp->cpl_apph);
            comp->recv_phase_open = 0;
        }
        if(comp->send_phase_open) {
            app_end_send_phase(comp->cpl_apph);
            comp->send_phase_open = 0;
        }
    }
}

/*
static int signal_status(struct benesh_handle *bnh, int leaving)
{
    static int sigid = 0;
    DEBUG_OUT("signal %i status is %i\n", sigid++, leaving);
    MPI_Allreduce(MPI_IN_PLACE, &leaving, 1, MPI_INT, MPI_MIN, bnh->mycomm);
    return(leaving);
}
*/

static void signal_minmax(struct benesh_handle *bnh, unsigned int signal, int *min, int *max) 
{
    static int sigid = 0;
    int sendbuf[2] = {signal, -signal};
    int recvbuf[2];

    DEBUG_OUT("signal %i status is %i\n", sigid++, signal);
    MPI_Allreduce(sendbuf, recvbuf, 2, MPI_INT, MPI_MIN, bnh->mycomm);
    if(min) {
        *min = recvbuf[0];
    }
    if(max) {
        *max = -recvbuf[1];
    }
}

int benesh_handle_work(struct benesh_handle *bnh)
{
    struct work_node *wnode;
    struct obj_entry *ent;
    int handled = 0;
    int result = 0;
    int bail = 0;

    APEX_FUNC_TIMER_START(benesh_handle_work);

    APEX_NAME_TIMER_START(1, "lock_work_bhwa");
    ABT_mutex_lock(bnh->work_mutex);
    APEX_TIMER_STOP(1);
    if(!bnh->wqueue_tail && bnh->comp_count) {
        APEX_NAME_TIMER_START(2, "wait_work");
        DEBUG_OUT("Work queue empty. Waiting for new work\n");
        ABT_cond_wait(bnh->work_cond, bnh->work_mutex);
        DEBUG_OUT("New work notification\n");
        APEX_TIMER_STOP(2);
    } else if(!bnh->comp_count) {
        DEBUG_OUT("Skipping wait even though the work queue is empty (comp_count is %i)\n", bnh->comp_count);
    }

    while(bnh->wqueue_tail) {
        // TODO: should try to detect unmet depedencies and wait, rather than
        // busy loop
        handled++;
        wnode = deque_work(bnh);
        ABT_mutex_unlock(bnh->work_mutex);
        result = handle_work(bnh, wnode);
        if(result == 0) {
            ABT_mutex_lock(bnh->work_mutex);
            benesh_make_active(bnh, wnode);
            ABT_mutex_unlock(bnh->work_mutex);
        } else if(result == 1) {
            switch(wnode->type) {
            case BNH_WORK_OBJ:
                DEBUG_OUT("cleanup from object completion\n");
                if(wnode->realize == 1) {
                    realize_object(bnh, wnode->tgt, wnode->var_maps);
                }
                activate_subs(bnh, wnode);
                break;
            case BNH_WORK_RULE:
                DEBUG_OUT("cleanup from subrule completion\n");
                APEX_NAME_TIMER_START(4, "lock_db_hwa");
                ABT_mutex_lock(bnh->db_mutex);
                APEX_TIMER_STOP(4);

                ent = get_object_entry(bnh, wnode->tgt, wnode->subrule,
                                       wnode->var_maps, 1);
                ent->realized = 1;
                ABT_mutex_unlock(bnh->db_mutex);
                activate_subs(bnh, wnode);
                break;
            case BNH_WORK_CHAIN:
                DEBUG_OUT("cleanup from chain completion\n");
                //TODO: make graceful - should be scheduled as a result of starting a phase
                //benesh_end_phases(bnh);
                break;
            }

            if(wnode->announce) {
                DEBUG_OUT("announcing work completion to other components\n");
                announce_work(bnh, wnode);
            } else {
                DEBUG_OUT("not announcing work completion.\n")
            }
        } else if(result == 3) {
            ABT_mutex_lock(bnh->work_mutex);
            benesh_make_active(bnh, wnode);
            ABT_mutex_unlock(bnh->work_mutex);
            DEBUG_OUT("canceling work handling loop with active work because some othe rank thinks we're done.\n");
            bail = 1;
            break;
        }
        APEX_NAME_TIMER_START(5, "lock_work_bhwc");
        ABT_mutex_lock(bnh->work_mutex);
        APEX_TIMER_STOP(5);
    }
    ABT_mutex_unlock(bnh->work_mutex);
    /*
    if(!bail) {
        signal_minmax(bnh, 0, NULL, NULL);
    }
    */
    DEBUG_OUT("handled %i work nodes\n", handled);

    APEX_TIMER_STOP(0);
    return(bail);
}

static int do_tpoint_rule(struct benesh_handle *bnh, struct tpoint_rule *rule,
                           int64_t *tp_vars)
{
    struct pq_obj **fq_tgt = malloc(sizeof(*fq_tgt) * rule->num_tgts);
    struct xc_list_node *tgt_obj;
    struct wf_target *tgt_rule;
    char **map;
    int64_t *map_vals;
    int found;
    int bail = 0, handle = 0;
    int i, j;

    APEX_FUNC_TIMER_START(tpoint_finished);
    for(i = 0; i < rule->num_tgts; i++) {
        tgt_obj = rule->tgts[i];
        fq_tgt[i] = resolve_obj(bnh, tgt_obj, rule->nmappings, rule->map_names,
                                tp_vars);
        if(bnh->f_debug) {
            DEBUG_OUT("target to realize for touchpoint: ");
            print_pq_obj_nl(stderr, fq_tgt[i]);
        }
        for(j = 0, found = 0; j < bnh->num_tgts; j++) {
            if(match_target_rule_fq(fq_tgt[i], &bnh->tgts[j], &map)) {
                DEBUG_OUT(" resolving with target rule %i\n", j);
                tgt_rule = &bnh->tgts[j];
                found = 1;
                break;
            }
        }
        if(!found) {
            fprintf(stderr, "ERROR: no target rule matching in finish check\n");
            free(map);
            APEX_TIMER_STOP(0);
            return (-1);
        }
        map_vals = malloc(sizeof(*map_vals) * tgt_rule->num_vars);
        for(j = 0; j < tgt_rule->num_vars; j++) {
            map_vals[j] = atoi(map[j]);
            DEBUG_OUT("  %s => %" PRIu64 "\n", tgt_rule->tgt_vars[j],
                      map_vals[j]);
        }
        // There's no ordering dependency to target generation in a touchpoint
        // rule,
        //  so the different targets should be being generated in parallel, not
        //  series. However, so far we only ever have one target per touchpoint.
        while(!object_realized(bnh, tgt_rule, map_vals)) {
            if(bail) {
                fprintf(stderr, "ERROR: we already bailed on the work handler because a different rank thinks we're done. But we're not done! This will probably cause a deadlock.\n");
            }
            handle = 1;
            bail = benesh_handle_work(bnh);
        }
        if(!bail) {
            signal_minmax(bnh, 0, NULL, NULL);
        }

        free(map);
        free(map_vals);
        free(fq_tgt);
    }
    APEX_TIMER_STOP(0);
}

static void do_ordered_send(struct benesh_handle *bnh, int comp_id, int var_id)
{
    struct wf_var *src_var = &bnh->ifvars[var_id];
    struct wf_domain *src_dom = src_var->dom;
    struct wf_component *dst_comp = &bnh->comps[comp_id];
    struct app_hndl *apph;
    struct field_handle *field;
    int i;

    if(src_dom->comm_type == BNH_COMM_RDV_CLI) {
        apph = bnh->comps[bnh->comp_id].cpl_apph;
    } else {
        apph = dst_comp->cpl_apph;
    }

    DEBUG_OUT("doing a field transfer (app %p)\n", (void *)apph);

    dst_comp->send_phase_open = 0; //DEBUG
    if(dst_comp->send_phase_open == 0) {
        DEBUG_OUT("starting send phase\n");
        app_begin_send_phase(apph);
        dst_comp->send_phase_open = 1;
    }
   
    DEBUG_OUT("%i fields to send\n", src_var->num_fields); 
    for(i = 0; i < src_var->num_fields; i++) {
        DEBUG_OUT("sending %s, field %i on %s using %p\n", src_var->name, i, src_dom->full_name, field);
        field = src_var->fields[i];
        cpl_send_field(field);
        DEBUG_OUT("sent\n");
    }

    app_end_send_phase(apph); // DEBUG!!!!
}

static void do_ordered_recv(struct benesh_handle *bnh, int comp_id, int var_id)
{
    struct wf_component *src_comp = &bnh->comps[comp_id];
    struct wf_var *dst_var = &bnh->ifvars[var_id];
    struct wf_domain *dst_dom = dst_var->dom;
    struct app_hndl *apph;
    struct field_handle *field;
    int i;

    if(dst_dom->comm_type == BNH_COMM_RDV_CLI) {
        apph = bnh->comps[bnh->comp_id].cpl_apph;
    } else {
        apph = src_comp->cpl_apph;
    }

    src_comp->recv_phase_open = 0; //DEBUG!!!
    if(src_comp->recv_phase_open == 0) {
        DEBUG_OUT("Starting receive phase (apph %p)\n", (void *)apph);
        app_begin_recv_phase(apph);
        src_comp->recv_phase_open = 1;
    }
    DEBUG_OUT("%i fields to get.\n", dst_var->num_fields);
    for(i = 0; i < dst_var->num_fields; i++) {
        field = dst_var->fields[i];
        DEBUG_OUT("receiving %s, field %i\n", dst_var->name, i);
        cpl_recv_field(field);
    }

    app_end_recv_phase(apph); //DEBUG!!!
}

static void take_nondummy_orders(struct benesh_handle *bnh)
{
    int cmd[3] = {0};
    do {
        if(bnh->grank == bnh->root_drank) {
            MPI_Recv(cmd, 3, MPI_INT, bnh->root_rank, 0, bnh->gcomm, MPI_STATUS_IGNORE);
            DEBUG_OUT("received new command (%i)\n", cmd[0]);
        }
        MPI_Bcast(cmd, 3, MPI_INT, 0, bnh->mycomm);
        DEBUG_OUT("doing command (%i, %i, %i)\n", cmd[0], cmd[1], cmd[2]);
        if(cmd[0] == BNH_FIELD_SEND) {
            do_ordered_send(bnh, cmd[1], cmd[2]);
        } else if(cmd[0] == BNH_FIELD_RECV) {
            do_ordered_recv(bnh, cmd[1], cmd[2]);
        }
    } while(cmd[0] != BNH_TERM);
    DEBUG_OUT("got term command\n");
}

void benesh_tpoint(struct benesh_handle *bnh, const char *tpname)
{
    struct tpoint_handle *tph = bnh->tph;
    char **tk_tpoint;
    int tkcnt;
    struct tpoint_rule *rule;
    struct tpoint_announce announce;
    int rule_id;
    struct tpoint_rule **rulep = &tph->rules;
    int64_t *values;
    int res;
    int found = 0;
    int i;

    DEBUG_OUT("starting touchpoint processing for %s\n", tpname);
    if(bnh->dummy) {
        take_nondummy_orders(bnh);
        DEBUG_OUT("finished dummy touchpoint processing for %s\n", tpname);
        return;
    }
    APEX_FUNC_TIMER_START(benesh_tpoint);

    tk_tpoint = tokenize_tpoint(tpname, &tkcnt);
    APEX_NAME_TIMER_START(1, "rule_matching");
    rule_id = 0;
    do {
        rule = &tph->rules[rule_id];
        if(rule->rule && rule->source &&
           match_rule(rule, tk_tpoint, tkcnt, &values)) {
            DEBUG_OUT(" matched rule %i, with mappings: \n", rule_id);
            if(bnh->f_debug) {
                for(i = 0; i < rule->nmappings; i++) {
                    DEBUG_OUT("   %s => %" PRIu64 "\n", rule->map_names[i],
                              values[i]);
                }
            }
            announce.rule_id = rule_id;
            announce.comp_id = bnh->comp_id;
            announce.tp_vars = values;
            APEX_NAME_TIMER_START(2, "ekt_tell_tpoint");
            ekt_tell(tph->ekth, NULL, bnh->tp_type, &announce);
            DEBUG_OUT("announced touchpoint %s\n", tpname);
            APEX_TIMER_STOP(2);
            found = 1;
            break;
        }
        rule_id++;
    } while(rule->rule);
    APEX_TIMER_STOP(1);

    do_tpoint_rule(bnh, rule, values);
    if(bnh->rank == 0 && bnh->root_drank > -1) {
        DEBUG_OUT("sending term command\n");
        command_dummies(bnh, BNH_TERM, -1, -1);
    }    

    DEBUG_OUT("finished touchpoint processing for %s\n", tpname);

    if(!found) {
        fprintf(stderr,
                "WARNING: %s tried to signal touchpoint %s, which is not a "
                "touchpoint for component %s. Ignoring.\n",
                bnh->name, tpname, bnh->comps[bnh->comp_id].name);
    }

    APEX_TIMER_STOP(0);
}

/*
static void report_cpl_timings(struct benesh_handle *bnh, struct wf_domain *dom_list, int dom_count)
{
    int i;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].field) {
            report_send_recv_timing(dom_list[i].field, dom_list[i].name);
        }
        if(dom_list[i].subdom_count) {
            report_cpl_timings(bnh, dom_list[i].subdoms, dom_list[i].subdom_count);
        }
    }
}
*/

void close_cpls(struct benesh_handle *bnh, struct wf_domain *dom_list, int dom_count)
{
    int i;
    
    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].cph) {
            close_cpl(dom_list[i].cph);
            if(dom_list[i].mesh) {
                close_mesh(dom_list[i].mesh);
            }
        }
        if(dom_list[i].subdom_count) {
            close_cpls(bnh, dom_list[i].subdoms, dom_list[i].subdom_count);
        }
    }
}

int benesh_fini(struct benesh_handle *bnh)
{
    uint32_t comp_id = bnh->comp_id;

    DEBUG_OUT("started fini\n");
    
    if(!bnh->dummy) {
        DEBUG_OUT("sending fini\n");
        ekt_tell(bnh->ekth, NULL, bnh->fini_type, &comp_id);
        DEBUG_OUT("sent fini. bnh->comp_count = %i\n", bnh->comp_count);
        while(bnh->comp_count) {
            benesh_handle_work(bnh);
        }
        DEBUG_OUT("all peers components finished\n");
    }
    MPI_Barrier(bnh->gcomm);
    //report_cpl_timings(bnh, bnh->doms, bnh->dom_count);
    close_cpls(bnh, bnh->doms, bnh->dom_count);
    if(bnh->rank == 0) {
        //dspaces_kill(bnh->dsp);
        DEBUG_OUT("did dspaces_kill\n");
    }
    //dspaces_fini(bnh->dsp);
    MPI_Barrier(bnh->gcomm);
    if(bnh->rank == 0) {
        DEBUG_OUT("did dspaces_fini\n");
    }
    bnsh_tpoint_fini(bnh->tph);
    MPI_Barrier(bnh->gcomm);
    if(bnh->rank == 0) {
        DEBUG_OUT("did bnsh_tpoint_fini\n");
    }
    if(!bnh->dummy) {
        ekt_fini(&bnh->ekth);
    }
    MPI_Barrier(bnh->gcomm);
    if(bnh->rank == 0) {
        DEBUG_OUT("did ekt_fini\n");
    }
    sleep(1);
    margo_finalize(bnh->mid);
    MPI_Barrier(bnh->gcomm);
    if(bnh->rank == 0) {
        DEBUG_OUT("did margo_finalize\n");
    }
    free(bnh->name);
    MPI_Comm_free(&bnh->gcomm);
    free(bnh);

    return(0);
}

int benesh_bind_method(struct benesh_handle *bnh, const char *name,
                       benesh_method method, void *user_arg)
{
    int i;

    DUMMY_OUT(0);

    for(i = 0; i < bnh->mth_count; i++) {
        if(strcmp(bnh->mths[i].name, name) == 0) {
            bnh->mths[i].method = method;
            bnh->mths[i].arg = user_arg;
            return 0;
        }
    }

    fprintf(stderr, "ERROR: %s method is unknown\n", name);
    return (-1);
}

static struct wf_var *match_local_ifvar(struct benesh_handle *bnh, const char *var_name)
{
    struct wf_var *var;
    int i, found;

    found = 0;
    for(i = 0; i < bnh->ifvar_count; i++) {
        var = &bnh->ifvars[i];
        if(var->comp_id == bnh->comp_id && strcmp(var->name, var_name) == 0) {
            found = 1;
            break;
        }
    }

    if(!found) {
        fprintf(stderr,"WARNING: trying to locate unknown interface variable '%s'.\n", var_name);
        return(NULL);
    }

    return(var);
} 

int benesh_bind_var(struct benesh_handle *bnh, const char *var_name, void *buf)
{
    struct wf_var *var;

    DUMMY_OUT(0);

    var = match_local_ifvar(bnh, var_name);
    if(var) {
        var->buf = buf;
    } else {
        return(-1);
    }

    return(0);
}

static void add_var_field(struct benesh_handle *bnh, struct wf_var *var, struct field_handle *field)
{
    var->fields = realloc(var->fields, sizeof(*var->fields) * ++var->num_fields);
    var->fields[var->num_fields-1] = field;
}

void *benesh_bind_var_mesh(struct benesh_handle *bnh, const char *var_name, int *idx, unsigned int idx_len)
{
    char *tmp_vname;
    char *app_name;
    char num_str[32];
    char *field_name, *adapt_name;
    struct wf_component *comp;
    struct wf_var *var;
    struct field_adapter *adpt;
    struct field_handle *field;
    struct wf_domain *dom;
    int i;
    int found = 0;

    DUMMY_OUT(0);

    DEBUG_OUT("binding var %s\n", var_name);

    if(idx_len > 1) {
        fprintf(stderr, "ERROR: one or zero variable index integers permitted for now.\n");
        return(NULL);
    }
    
    tmp_vname = strdup(var_name);
    app_name = strchr(tmp_vname, '\\');
    if(app_name) {
        *app_name = '\0';
        app_name++;    
    } else {
        fprintf(stderr, "ERROR: mesh variables must indicate partner component, for now.\n");
        return(NULL);
    }

    for(i = 0; i < bnh->comp_count; i++) {
        comp = &bnh->comps[i];
        if(strcmp(comp->app, app_name) == 0) {
            found = 1;
            break;
        }
    }
    if(!found) {
        fprintf(stderr, "ERROR: could not find workflow component named '%s'\n", app_name);
    }

    var = match_local_ifvar(bnh, var_name);
    if(var) {
        dom = var->dom;
    } else {
        return(NULL);
    }

    if(idx_len == 1) {
        sprintf(num_str, "%i", idx[0]);
        field_name = malloc(strlen(tmp_vname) + strlen(num_str) + 2);
        sprintf(field_name, "%s_%s", tmp_vname, num_str);   
    } else {
        field_name = tmp_vname;
    }
    DEBUG_OUT("field name is %s\n", field_name);
    adapt_name = malloc(strlen(app_name) + strlen(field_name) + 2);
    sprintf(adapt_name, "%s/%s", app_name, field_name);
    DEBUG_OUT("adapter name is %s\n", adapt_name);
    //TODO data type
    adpt = create_omegah_adapter(comp->cpl_apph, field_name, adapt_name, dom->mesh, BNH_CPL_DOUBLE);
    DEBUG_OUT("created adapter\n");
    field = cpl_add_field(dom->cph, app_name, field_name, 1);
    DEBUG_OUT("added field\n");
    add_var_field(bnh, var, field);
    if(idx_len == 1) {
        free(field_name);
    }
    free(adapt_name);
    free(tmp_vname);

    return(cpl_get_field_ptr(field));
}

// TODO: do internally based on configuration - tricky if supporting dummy handles
int benesh_bind_field_domain(struct benesh_handle *bnh, const char *dom_name)
{
    struct wf_domain *dom;
    struct wf_component *comp;

    DEBUG_OUT("binding domain %s to support opaque fields.\n", dom_name);

    /*
    if(bnh->dummy) {
        DEBUG_OUT("I have a dummy handle. Allocate minimal structures for later field bindings.\n");
        dom = malloc(sizeof(*dom));
        dom->type = BNH_DOM_MESH;
        dom->comm_type = BNH_COMM_RDV_CLI;
        dom->name = strdup(dom_name);
        comp = malloc(sizeof(*comp));
        bnh->dummy_dom = dom;
        bnh->dummy_comp = comp;
    } else {
        DEBUG_OUT("I am a full-fledged handle.\n");
        dom = match_domain(bnh, dom_name);
        comp = &bnh->comps[bnh->comp_id];
    }
    */
    dom = match_domain(bnh, dom_name);
    comp = &bnh->comps[bnh->comp_id];
    if(dom->type != BNH_DOM_MESH) {
        fprintf(stderr,
                "ERROR: binding a field to grid '%s' is not implemented yet.\n",
                dom_name);
        return (-1);
    }

    if(dom->comm_type == BNH_COMM_RDV_CLI) {
        dom->cph = create_cpl_hndl(bnh->name, NULL, NULL, 0, bnh->dummy ? MPI_COMM_NULL : bnh->mycomm);
        comp->cpl_apph = add_application(dom->cph, "server", "");
    } else if(dom->comm_type == BNH_COMM_RDV_SRV) {
        fprintf(stderr, "ERROR: raw field domain binding not supported on servers yet.\n");
        return(-1);
    } else {
        fprintf(stderr, "ERROR: raw field domain binding requires rendezvous transport.\n");
        return(-1);
    }
    
    return(0); 
}

static struct wf_var *get_or_create_new_dummy(struct benesh_handle *bnh, const char *var_name)
{
    struct wf_var *var = NULL;
    int i;

    DEBUG_OUT("get or create %s\n", var_name);

    for(i = 0; i < bnh->num_dummy_vars; i++) {
        var = &bnh->dummy_vars[i];
        if(strcmp(var->name, var_name) == 0) {
            DEBUG_OUT("returning existing dummy var\n");
            return(var);
        }
    }

    DEBUG_OUT("dummy var %s doesn't already exist. Creating new...\n", var_name);
    bnh->dummy_vars = realloc(bnh->dummy_vars,
                (bnh->num_dummy_vars+1) * sizeof(*bnh->dummy_vars));
    var = &bnh->dummy_vars[bnh->num_dummy_vars++];
    var->name = strdup(var_name);
    var->num_fields = 0;
    var->fields = NULL;
    DEBUG_OUT("done creating new var\n");

    return(var);
}

// TODO support server
void *benesh_bind_field_mpient(struct benesh_handle *bnh, const char *var_name, int idx, const char *rcn_file, MPI_Comm comm, void *buffer, int length, int participates)
{
    struct wf_domain *dom;
    struct wf_var *var;
    struct wf_component *comp;
    struct field_adapter *adpt;
    struct field_handle *field;
    struct rcn_handle *rcn;
    char *field_name; 
    char num_str[32];
    int i, found;

    DEBUG_OUT("binding field '%s', idx %i (does%s participate)\n", var_name, idx, participates ? "":" not");

    /*
    if(bnh->dummy) {
        var = get_or_create_new_dummy(bnh, var_name);
        dom = bnh->dummy_dom;
        comp = bnh->dummy_comp;
    } else {
        comp = &bnh->comps[bnh->comp_id];
        var = match_local_ifvar(bnh, var_name);
        if(var) {
            dom = var->dom;
        } else {
            return(NULL);
        }
    }
    */
    comp = &bnh->comps[bnh->comp_id];
    var = match_local_ifvar(bnh, var_name);
    if(var) {
        dom = var->dom;
    } else {
        return(NULL);
    }

    // TODO: drop client assumption

    if(idx >= 0) {
        sprintf(num_str, "%i", idx);
        field_name = malloc(strlen(var_name) + strlen(num_str) + 2);
        sprintf(field_name, "%s_%s", var_name, num_str);
    } else {
        field_name = strdup(var_name);
    }

    rcn = get_rcn_from_file(rcn_file, MPI_COMM_SELF);
    // TODO data type
    adpt = create_mpient_adapter(comp->cpl_apph, field_name, rcn, comm, buffer, length, BNH_CPL_DOUBLE, 0, -1);
    field = cpl_add_field(dom->cph, "server", field_name, participates);
    add_var_field(bnh, var, field);

    free(field_name);

    return(cpl_get_field_ptr(field));
}

void *benesh_bind_field_dummy(struct benesh_handle *bnh, const char *var_name, int idx, int participates)
{
    struct wf_domain *dom;
    struct wf_var *var;
    struct wf_component *comp;
    struct field_adapter *adpt;
    struct field_handle *field;
    char *field_name;
    char num_str[32];
    int i, found;

    DEBUG_OUT("dummy binding field '%s', idx %i (does%s participate)\n", var_name, idx, participates ? "":" not");

    /*
    if(bnh->dummy) {
        DEBUG_OUT("I am, myself, a dummy handle.\n")
        comp = bnh->dummy_comp;
        var = get_or_create_new_dummy(bnh, var_name);
        dom = bnh->dummy_dom;
    } else {
        var = match_local_ifvar(bnh, var_name);
        if(var) {
            dom = var->dom;
        } else {
            return(NULL);
        }
        comp = &bnh->comps[bnh->comp_id];
    }
    */
    var = match_local_ifvar(bnh, var_name);
    if(var) {
        dom = var->dom;
    } else {
        return(NULL);
    }
    comp = &bnh->comps[bnh->comp_id];
  
    // TODO: drop client assumption

    if(idx >= 0) {
        DEBUG_OUT("a non-negative index was passed...add this to the field name.\n");
        sprintf(num_str, "%i", idx);
        field_name = malloc(strlen(var_name) + strlen(num_str) + 2);
        sprintf(field_name, "%s_%s", var_name, num_str);
    } else {
        field_name = strdup(var_name);
    }
    DEBUG_OUT("field name is %s\n", field_name);
    adpt = create_dummy_adapter(comp->cpl_apph, field_name);
    field = cpl_add_field(dom->cph, "server", field_name, participates);
    DEBUG_OUT("add new field to variable\n");
    add_var_field(bnh, var, field);

    free(field_name);

    return(cpl_get_field_ptr(field));
}

static int same_root_domain(struct wf_domain *dom1, struct wf_domain *dom2)
{
    char *name1, *name2;

    name1 = dom1->full_name;
    name2 = dom2->full_name;

    while(1) {
        if(*name1 != *name2) {
            return (0);
        }
        if(*name1 == '.' || *name1 == '\0') {
            return (1);
        }
        name1++;
        name2++;
    }

    fprintf(stderr, "ERROR: bugged out comparing root of %s and %s\n",
            dom1->full_name, dom2->full_name);
}

struct wf_domain *match_domain(struct benesh_handle *bnh, const char *dom_name)
{
    struct xc_list_node *obj;
    struct wf_domain *dom, *dom_list;
    int obj_len, dom_list_len;
    int i;

    obj = xc_strto_obj(dom_name);
    dom_list = bnh->doms;
    dom_list_len = bnh->dom_count;

    while(obj) {
        if(!dom_list) {
            return (NULL);
        }
        for(i = 0; i < dom_list_len; i++) {
            dom = &dom_list[i];
            if(strcmp(dom->name, obj->decl) == 0) {
                dom_list = dom->subdoms;
                dom_list_len = dom->subdom_count;
                break;
            }
        }
        obj = obj->next;
    }
    xc_free_obj(obj);

    return (dom);
}

int get_rank(double glb, double gub, int rdvRanks, double gpt)
{
    return ((int)(((gpt - glb) / (gub - glb)) * rdvRanks));
}

void get_rdv_dests(struct benesh_handle *bnh, double glb, double gub,
                   int rdvRanks, double llb, double lub, uint64_t pts,
                   uint32_t **dest, uint32_t **offset, size_t *count)
{
    double pitch;
    double gpt;
    long i;
    int pos, rank;

    DEBUG_OUT("calculating rendzevous distribution for the the [%lf, %lf] "
              "subset of [%lf, %lf] with %i rdv ranks and %li grid points.\n",
              llb, lub, glb, gub, rdvRanks, pts);
    pitch = (lub - llb) / pts;
    DEBUG_OUT("pitch is %lf\n", pitch);

    *count = 1;
    for(i = 0; i < pts - 1; i++) {
        if(get_rank(glb, gub, rdvRanks, llb + (pitch * i)) !=
           get_rank(glb, gub, rdvRanks, llb + (pitch * (i + 1)))) {
            (*count)++;
            DEBUG_OUT("found new cut ending at %li\n", i);
        }
    }

    *dest = malloc(*count * sizeof(**dest));
    *offset = malloc((*count + 1) * sizeof(**offset));

    (*offset)[0] = 0;
    (*dest)[0] = get_rank(glb, gub, rdvRanks, llb);
    pos = 1;
    for(i = 1; i < pts; i++) {
        rank = get_rank(glb, gub, rdvRanks, llb + (pitch * i));
        if(rank != (*dest)[pos - 1]) {
            (*dest)[pos] = rank;
            (*offset)[pos] = i;
            pos++;
        }
    }
    (*offset)[pos] = pts;
}

int cpl_add_fields(struct benesh_handle *bnh, struct wf_domain *dom,
                  struct wf_domain *dom_list, int dom_count, int pos)
{
    int i;
    int count = 0, count_once;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].comm_type == BNH_COMM_RDV_CLI &&
           same_root_domain(dom, &dom_list[i])) {
            DEBUG_OUT("adding field %s for app %s\n", "gid", dom->name);
            //dom_list[i].field = create_gid_field(dom_list[i].name, "gid", dom->cph, dom->mesh, NULL);
            //DEBUG_OUT("created field %p\n", (void *)(dom_list[i].field));
            pos++;
            count++;
        }
        if(dom_list[i].subdom_count) {
            count_once = cpl_add_fields(bnh, dom, dom_list[i].subdoms,
                                       dom_list[i].subdom_count, pos);
            count += count_once;
            pos += count_once;
        }
    }

    return (count);
}

int allocate_rdvs(struct benesh_handle *bnh, struct wf_domain *dom,
                  struct wf_domain *dom_list, int dom_count, int pos)
{
    int i;
    int count = 0, count_once;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].comm_type == BNH_COMM_RDV_CLI &&
           same_root_domain(dom, &dom_list[i])) {
            DEBUG_OUT("rdv server establishing communication on domain '%s'\n",
                      dom_list[i].full_name);
            //dom_list[i].rdv = malloc(sizeof(*dom_list[i].rdv));
            //dom_list[i].rdv = new_rdv_comm_ptn(
            //    &bnh->mycomm, dom_list[i].full_name, 1, dom->rptn);
            pos++;
            count++;
        }
        if(dom_list[i].subdom_count) {
            count_once = allocate_rdvs(bnh, dom, dom_list[i].subdoms,
                                       dom_list[i].subdom_count, pos);
            count += count_once;
            pos += count_once;
        }
    }

    return (count);
}

int find_client_domain_count(struct benesh_handle *bnh, struct wf_domain *dom,
                             struct wf_domain *dom_list, int dom_count)
{
    int i;
    int count = 0;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].comm_type == BNH_COMM_RDV_CLI &&
           same_root_domain(dom, &dom_list[i])) {
            count++;
        }
        if(dom_list[i].subdom_count) {
            count += find_client_domain_count(bnh, dom, dom_list[i].subdoms,
                                              dom_list[i].subdom_count);
        }
    }

    return (count);
}

void update_layouts(struct benesh_handle *bnh, struct wf_domain *dom,
                    struct wf_domain *dom_list, int dom_count)
{
    int i;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].comm_type == BNH_COMM_RDV_CLI && same_root_domain(dom, &dom_list[i])) {
            //rdv_layout(dom_list[i].rdv, dom->rdv_dst_count, dom->rdv_dest, dom->rdv_offset);
        }
        if(dom_list[i].subdom_count) {
            update_layouts(bnh, dom, dom_list[i].subdoms, dom_list[i].subdom_count);
        }
    }
}

struct wf_domain *find_server_dom(struct benesh_handle *bnh, struct wf_domain *dom,
                                    struct wf_domain *dom_list, int dom_count)
{
    struct wf_domain *ret_dom;
    int i;

    for(i = 0; i < dom_count; i++) {
        if(dom_list[i].comm_type == BNH_COMM_RDV_SRV && same_root_domain(dom, &dom_list[i])) {
            return(&dom_list[i]);
        }
        if(dom_list[i].subdom_count) {
            ret_dom = find_server_dom(bnh, dom, dom_list[i].subdoms, dom_list[i].subdom_count);
            if(ret_dom) {
                return(ret_dom);
            }
        }
    }

    return(NULL);
}

int benesh_bind_mesh_domain(struct benesh_handle *bnh, const char *dom_name,
         const char *grid_file, const char *cpn_file, int alloc)
{
    char *path;
    struct wf_domain *dom;
    struct wf_component *comp;
    int i;

    DUMMY_OUT(0);

    dom = match_domain(bnh, dom_name);
    if(dom->type != BNH_DOM_MESH) {
        fprintf(stderr,
                "ERROR: binding a mesh to grid '%s' is not implemented yet.\n",
                dom_name);
        return (-1);
    }

    dom->mesh = new_oh_mesh(grid_file);
    dom->rptn = create_oh_partition(dom->mesh, cpn_file);
    //TODO wfname
    if(dom->comm_type == BNH_COMM_RDV_CLI) {
        dom->cph = create_cpl_hndl(bnh->name, dom->mesh, dom->rptn, 0, bnh->mycomm);
        comp = &bnh->comps[bnh->comp_id];
        comp->cpl_apph = add_application(dom->cph, "server", "");
        //TODO class ranges (here and below)
        mark_cpl_overlap(dom->cph, comp->cpl_apph, dom->mesh, dom->rptn, 0, 0);
    } else if(dom->comm_type == BNH_COMM_RDV_SRV) {
        dom->cph = create_cpl_hndl("xgc_n0_coupling", dom->mesh, dom->rptn, 1, bnh->mycomm);
        for(i = 0; i < bnh->comp_count; i++) {
            if(i != bnh->comp_id) {
                comp = &bnh->comps[i];
                path = malloc(strlen(comp->app) + 2);
                sprintf(path, "%s/", comp->app);
                comp->cpl_apph = add_application(dom->cph, comp->app, path);
                mark_cpl_overlap(dom->cph, comp->cpl_apph, dom->mesh, dom->rptn, 0, 0);
                free(path);
            }
        }
    } else {
        fprintf(stderr, "ERROR: mesh binding requires rendezvous transport.\n");
        return (-1);
    }

}

int benesh_bind_grid_domain(struct benesh_handle *bnh, const char *dom_name,
                            double *grid_offset, double *grid_dims,
                            uint64_t *grid_points, int alloc)
{
    struct wf_domain *dom;
    struct wf_var *var;
    size_t grid_size = 1;
    int i, j;

    DUMMY_OUT(0);

    dom = match_domain(bnh, dom_name);
    if(dom->type != BNH_DOM_GRID) {
        fprintf(stderr,
                "ERROR: binding grid dimensions to mesh '%s' is not "
                "implemented yet.\n",
                dom_name);
        return (-1);
    }
    dom->l_offset = malloc(sizeof(*dom->l_offset) * dom->dim);
    // TODO: move this to config file - it should be global
    dom->l_grid_dims = malloc(sizeof(*dom->l_grid_dims) * dom->dim);

    if(grid_points) {
        // if NULL, match writing component
        dom->l_grid_pts = malloc(sizeof(*dom->l_grid_pts) * dom->dim);
        memcpy(dom->l_grid_pts, grid_points,
               sizeof(*dom->l_grid_pts) * dom->dim);
        for(i = 0; i < dom->dim; i++) {
            DEBUG_OUT("%li grid points in dimension %i\n", dom->l_grid_pts[i],
                      i);
            grid_size *= dom->l_grid_pts[i];
        }
        if(alloc) {
            for(i = 0; i < bnh->ifvar_count; i++) {
                var = &bnh->ifvars[i];
                if(var->dom == dom) {
                    // TODO: support types properly
                    var->buf_size = sizeof(double) * grid_size;
                    if(var->buf) {
                        free(var->buf);
                    }
                    DEBUG_OUT(
                        "allocating buffer of size %li bytes for var %s\n",
                        var->buf_size, var->name);
                    var->buf = malloc(var->buf_size);
                }
            }
        }
        if(bnh->rdvRanks) {
            if(dom->dim != 1) {
                fprintf(stderr,
                        "ERROR: rdv only works for dim = 1 right now\n");
            } else {
                get_rdv_dests(bnh, dom->lb[0], dom->ub[0], bnh->rdvRanks,
                              grid_offset[0] + dom->lb[0],
                              grid_offset[0] + dom->lb[0] + grid_dims[0],
                              grid_points[0], &dom->rdv_dest, &dom->rdv_offset,
                              &dom->rdv_count);
            }
        }
    }

    memcpy(dom->l_offset, grid_offset, sizeof(*dom->l_offset) * dom->dim);
    memcpy(dom->l_grid_dims, grid_dims, sizeof(*dom->l_grid_dims) * dom->dim);

    return (0);
}

int benesh_get_var_domain(struct benesh_handle *bnh, const char *var_name,
                          char **dom_name, int *ndim, double **lb, double **ub)
{
    struct wf_var *var;
    struct wf_domain *dom;

    DUMMY_OUT(0);

    DEBUG_OUT("searching ifvar for '%s'\n", var_name);
    var = get_ifvar(bnh, var_name, bnh->comp_id, NULL);
    DEBUG_OUT("found at %p\n", (void *)var);
    dom = var->dom;
    DEBUG_OUT("var has domain '%s'\n", dom->full_name);
    *dom_name = strdup(dom->full_name);
    if(dom->type == BNH_DOM_MESH) {
        DEBUG_OUT("mesh type domain\n");
        return (0);
    }
    DEBUG_OUT("grid type domain\n");
    if(ndim) {
        *ndim = dom->dim;
    }
    if(lb) {
        *lb = malloc(sizeof(**lb) * *ndim);
        memcpy(*lb, dom->lb, sizeof(**lb) * *ndim);
    }
    if(ub) {
        *ub = malloc(sizeof(**ub) * *ndim);
        memcpy(*ub, dom->ub, sizeof(**ub) * *ndim);
    }

    return (0);
}

void *benesh_get_var_buf(struct benesh_handle *bnh, const char *var_name, uint64_t *size)
{
    struct wf_var *var;

    if(bnh->dummy) {
        fprintf(stderr, "WARNING: requested variable buffer '%s' from dummy handle.\n", var_name);
        return(NULL);
    }

    var = get_ifvar(bnh, var_name, bnh->comp_id, NULL);
    if(size) {
        *size = var->buf_size;
    }

    return (var->buf);
}

double benesh_get_var_val(struct benesh_handle *bnh, const char *var_name)
{
    struct wf_var *var;

    if(bnh->dummy) {
        fprintf(stderr, "WARNING: requested workflow value '%s' from dummy handle.\n", var_name);
        return(0);
    }

    var = get_gvar(bnh, var_name);

    return (var->val);
}

void benesh_unify_mesh_data(struct benesh_handle *bnh, const char *var_name)
{
    struct wf_var *var;
    struct wf_domain *dom;
    const char **field_names;
    int i;

    DUMMY_OUT();

    var = get_ifvar(bnh, var_name, bnh->comp_id, NULL);
    dom = var->dom;

    if(dom->type != BNH_DOM_MESH) {
        fprintf(stderr, "ERROR: grid unification not implmented yet.\n");
        return;
    }
    
    if(dom->comm_type == BNH_COMM_RDV_CLI) {        
        fprintf(stderr, "WARNING: trying to unify a server comm.\n");
        return;
    } else if(dom->comm_type == BNH_COMM_DSP) {
        fprintf(stderr, "WARNING: cannot unify dataspaces variables yet.\n");
        return;
    }

    field_names = malloc(dom->subdom_count * sizeof(*field_names));
    for(i = 0; i < dom->subdom_count; i++) {
        if(dom->subdoms[i].comm_type == BNH_COMM_RDV_CLI) {
            DEBUG_OUT("Adding field %s\n", dom->subdoms[i].name);
            field_names[i] = dom->subdoms[i].name;
        }
    }

    //cpl_combine_fields(dom->cph, dom->subdom_count, field_names);
    free(field_names);
}

#endif /* _BENESH_H_ */
