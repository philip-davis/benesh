#include <abt.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-obj.h"
#include "benesh-targets.h"
#include "benesh.h"
#include "beneshp.h"

#include "util.h"

struct benesh_dir_assign;
struct benesh_dir_method;
struct benesh_dir_put;
struct benesh_dir_get;
struct benesh_dir_inject;
struct beensh_dir_complete_obj;
struct benesh_dir_announce;

struct benesh_directive {
    enum bnh_task_type type;
    struct benesh_target *tgt;
    struct benesh_component *comp;
    union {
        struct benesh_dir_assign *assign;
        struct benesh_dir_method *method;
        struct benesh_dir_put *put;
        struct benesh_dir_get *get;
        struct benesh_dir_inject *inject;
        struct beensh_dir_complete_obj *complete_obj;
        struct benesh_dir_announce *announce;
    };
};

struct benesh_rule {
    int id; // needed for db lookup
    int nvar;
    struct benesh_obj *target;
    struct bnh_pvec *prereqs;
    struct bnh_pvec *directives;
};

struct benesh_target {
    _Atomic enum bnh_tgt_status status;
    struct bnh_pvec *dependent_tasks;
    struct bnh_pvec *directive_tasks;
    struct benesh_rule *rule;
    int dir_complete;
    int64_t *var_map;
};

struct benesh_target_db {
    ABT_mutex mtx;
    struct bnh_hash *entries;
};

int benesh_rule_get_nvar(struct benesh_rule *rule, size_t *nvar)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    if(!nvar) {
        ERR_OUT(BNH_EFAULT, err_out, "bad return pointer.\n");
    }
    *nvar = rule->nvar;

    return (0);
err_out:
    *nvar = 0;
    return (err);
}

struct bnh_pvec *benesh_rule_get_directives(struct benesh_rule *rule)
{
    TRACE_OUT;
    int err;

    if(!rule || !rule->directives) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }

    return (rule->directives);

err_out:
    return (NULL);
}

struct bnh_pvec *benesh_rule_get_prereqs(struct benesh_rule *rule)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }

    return (rule->prereqs);
err_out:
    return (NULL);
}
/*
    foo.%{x}.%{y}: bar.%{y}.3
      ...

    bar.%{x}.%{y}
      ...

    baz@qux:
      foo.1.2

*/
struct bnh_pvec *benesh_target_get_prereqs(struct benesh_handle *bnh,
                                           struct benesh_target *tgt)
{
    TRACE_OUT;
    struct benesh_rule *rule;
    struct bnh_pvec *resolved_prereqs;
    struct benesh_obj *prereq_obj;
    struct benesh_target *prereq;
    bnh_pvec_iter bi;
    size_t nprereq;
    int err;

    if(!tgt || !tgt->rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }

    rule = tgt->rule;
    CHECK_ZERO(benesh_target_get_nprereq(tgt, &nprereq), err, err_out,
               "could not access target.\n");
    resolved_prereqs = bnh_pvec_new(nprereq);
    BNH_PVEC_FOREACH(prereq_obj, bi, rule->prereqs)
    {
        ASSIGN_NOT_NULL(
            benesh_obj_resolve_to_tgt(bnh, prereq_obj, tgt->var_map), prereq,
            BNH_ENOENT, err_out, "could not resolve object to target.\n");
        bnh_pvec_append(resolved_prereqs, prereq, -1);
    }

    return (resolved_prereqs);
err_out:
    return (NULL);
}

int benesh_rule_get_nprereq(struct benesh_rule *rule, size_t *nprereq)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    if(!nprereq) {
        ERR_OUT(BNH_EFAULT, err_out, "bad return pointer.\n");
    }

    *nprereq = bnh_pvec_get_len(rule->prereqs);

    return (0);
err_out:
    *nprereq = 0;
    return (err);
}

int benesh_target_get_nprereq(struct benesh_target *tgt, size_t *nprereq)
{
    TRACE_OUT;
    struct benesh_rule *rule;
    int err;

    if(!tgt || !tgt->rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    if(!nprereq) {
        ERR_OUT(BNH_EFAULT, err_out, "bad return pointer.\n");
    }

    rule = tgt->rule;
    *nprereq = bnh_pvec_get_len(rule->prereqs);

    return (0);
err_out:
    *nprereq = 0;
    return (err);
}

struct bnh_pvec *benesh_target_get_directives(struct benesh_target *tgt)
{
    TRACE_OUT;
    struct benesh_rule *rule;
    int err;

    if(!tgt || !tgt->rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }

    rule = tgt->rule;
    return (benesh_rule_get_directives(rule));

err_out:
    return (NULL);
}

int benesh_target_get_ndir(struct benesh_target *tgt, size_t *ndir)
{
    TRACE_OUT;
    struct benesh_rule *rule;
    int err;

    if(!tgt || !tgt->rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    if(!ndir) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    rule = tgt->rule;
    CHECK_ZERO(benesh_rule_get_ndir(rule, ndir), err, err_out,
               "could not access rule.\n");

    return (0);
err_out:
    *ndir = -1;
    return (err);
}

int benesh_rule_get_ndir(struct benesh_rule *rule, size_t *ndir)
{
    TRACE_OUT;
    int err;

    if(!rule || !rule->directives) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    if(!ndir) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    *ndir = bnh_pvec_get_len(rule->directives);

    return (0);
err_out:
    *ndir = -1;
    return (err);
}

int benesh_get_num_rules(benesh_rulebook rules)
{
    TRACE_OUT;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }
    return (bnh_pvec_get_len(rules));
err_out:
    return (-1);
}

struct benesh_rule *benesh_rule_get_by_id(benesh_rulebook rules, int rule_id)
{
    TRACE_OUT;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }

    return (bnh_pvec_get_by_id(rules, rule_id));

err_out:
    return (NULL);
}

int benesh_rule_find_viable(benesh_rulebook rules, struct benesh_obj *obj,
                            struct benesh_rule **rule, int64_t **var_map)
{
    TRACE_OUT;
    struct benesh_rule *br;
    bnh_pvec_iter bi;
    int is_match = 0;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }
    if(!rule || !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output target.\n");
    }

    BNH_PVEC_FOREACH(br, bi, rules)
    {
        CHECK_ZERO(benesh_unify_obj_target(br->target, obj, var_map, &is_match),
                   err, err_out, "error during rule matching.\n");
        if(is_match) {
            *rule = br;
            break;
        }
    }

    return (0);
err_out:
    return (err);
}

struct benesh_target *benesh_target_init(struct benesh_rule *rule,
                                         int64_t *var_map,
                                         enum bnh_tgt_status status)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    size_t nvar, ndir;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), BNH_ESTATE, err_out,
               "could not access rule.\n");
    if(nvar && !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad var map.\n");
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*tgt)), tgt, BNH_ENOMEM, err_out,
                    "buffer allocation error.\n");
    CHECK_ZERO(benesh_rule_get_ndir(rule, &ndir), BNH_ESTATE, err_out,
               "could not access rule.\n");
    ASSIGN_NOT_NULL(bnh_pvec_new(ndir), tgt->directive_tasks, BNH_ENOMEM,
                    err_out, "could not create directive task vector.\n");
    ASSIGN_NOT_NULL(bnh_pvec_new(2), tgt->dependent_tasks, BNH_ENOMEM, err_out,
                    "could not create dependent task vector.\n");
    tgt->rule = rule;
    ASSIGN_NOT_NULL(malloc(sizeof(*tgt->var_map) * nvar), tgt->var_map,
                    BNH_ENOMEM, err_out, "failed to allocate buffer.\n");
    memcpy(tgt->var_map, var_map, sizeof(*tgt->var_map) * nvar);
    tgt->status = status;
    tgt->dir_complete = 0;

    return (tgt);
err_out:
    return (NULL);
}

struct benesh_target_db *benesh_db_init(benesh_rulebook rules)
{
    TRACE_OUT;
    struct benesh_target_db *bdb;
    int nrules;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }
    nrules = benesh_get_num_rules(rules);
    bdb = malloc(sizeof(*bdb));
    ABT_mutex_create(&bdb->mtx);
    bdb->entries = bnh_hash_new(nrules, 1);

    return (bdb);
err_out:
    return (NULL);
}

int benesh_tgt_db_lock(struct benesh_target_db *bdb)
{
    TRACE_OUT;
    int err;

    if(!bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid target database handle.\n");
    }

    CHECK_ZERO(ABT_mutex_lock(bdb->mtx), BNH_EABT, err_out,
               "mutex lock failed with %i\n", err);

    return (0);

err_out:
    return (err);
}

int benesh_tgt_db_unlock(struct benesh_target_db *bdb)
{
    TRACE_OUT;
    int err;

    if(!bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid target database handle.\n");
    }

    CHECK_ZERO(ABT_mutex_unlock(bdb->mtx), BNH_EABT, err_out,
               "mutex unlock failed with %i\n", err);

    return (0);
err_out:
    return (err);
}

static struct bnh_hash *benesh_tgt_db_vm_lookup(struct bnh_hash *tgt_hash,
                                                int64_t *var_maps, size_t nvar)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    struct bnh_hash *next_hash;
    int err;

    if(!tgt_hash) {
        ERR_OUT(BNH_EFAULT, err_out, "bad hash table.\n");
    }

    if(nvar == 1) {
        return (tgt_hash);
    }
    next_hash = bnh_hash_lookup(tgt_hash, var_maps[0]);
    if(!next_hash) {
        ASSIGN_NOT_NULL(bnh_hash_new(16, 1), next_hash, BNH_ENOMEM, err_out,
                        "hash allocation failure.\n");
        bnh_hash_add_entry(tgt_hash, var_maps[0], next_hash);
    }

    return (benesh_tgt_db_vm_lookup(next_hash, &var_maps[1], nvar - 1));

err_out:
    return (NULL);
}

struct benesh_target *benesh_tgt_db_lookup(struct benesh_target_db *bdb,
                                           struct benesh_rule *rule,
                                           int64_t *var_map)
{
    TRACE_OUT;
    struct benesh_target *tgt;
    struct bnh_hash *tgt_hash, *tgt_hash_next;
    int id;
    size_t nvar;
    int err, err2;

    if(!bdb) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target db handle.\n");
    }
    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }

    CHECK_ZERO(benesh_tgt_db_lock(bdb), err, err_out,
               "could not lock target db.\n");
    CHECK_ZERO(benesh_rule_get_nvar(rule, &nvar), err, err_out_lock,
               "could not access rule.\n");
    if(!nvar) {
        /*
            Only a single target for this rule. No nested hash structure.
        */
        tgt_hash = bdb->entries;
        id = rule->id;
    } else {
        /*
            Traverse the nested hash structure.
        */
        tgt_hash = bnh_hash_lookup(bdb->entries, rule->id);
        if(!tgt_hash) {
            ASSIGN_NOT_NULL(bnh_hash_new(16, 1), tgt_hash, BNH_ENOMEM,
                            err_out_lock, "hash allocation failure.\n");
            bnh_hash_add_entry(bdb->entries, rule->id, tgt_hash);
        }
        // step through var_map, creating new nested hash tables as necessary.
        tgt_hash = benesh_tgt_db_vm_lookup(tgt_hash, var_map, nvar);
        // last hash layer points to the target structure
        id = var_map[nvar - 1];
    }
    tgt = bnh_hash_lookup(tgt_hash, id);
    if(!tgt) {
        ASSIGN_NOT_NULL(benesh_target_init(rule, var_map, BNH_STAT_UNREAL), tgt,
                        BNH_ENOMEM, err_out_lock,
                        "target allocation failure.\n");
        bnh_hash_add_entry(tgt_hash, id, tgt);
    }
    CHECK_ZERO(benesh_tgt_db_unlock(bdb), err, err_out,
               "could not unlock target db. Possible deadlock!\n");

    return (tgt);
err_out_lock:
    // successful unlock clears err. Save it.
    err2 = err;
    CHECK_ZERO(benesh_tgt_db_unlock(bdb), err, err_out,
               "could not unlock target db. Possible deadlock!\n");
    err = err2;
err_out:
    return (NULL);
}

int benesh_target_get_status(struct benesh_target *tgt, int *status)
{
    TRACE_OUT;
    int err;

    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    if(!status) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    *status = tgt->status;

    return (0);
err_out:
    return (err);
}

int benesh_target_dir_status(struct benesh_target *tgt, int dir_id, int *status)
{
    TRACE_OUT;
    size_t ndir;
    int err;

    if(!tgt) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    CHECK_ZERO(benesh_target_get_ndir(tgt, &ndir), BNH_ESTATE, err_out,
               "could not access target.\n");
    if(dir_id < 0 || dir_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "invalid directive id.\n");
    }
    if(!status) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    if(tgt->dir_complete > dir_id) {
        *status = BNH_DIR_STAT_DONE;
    } else {
        *status = BNH_DIR_STAT_NOT_SCHED;
        if(bnh_pvec_get_by_id(tgt->directive_tasks, dir_id)) {
            *status = BNH_DIR_STAT_SCHED;
        }
    }

    return (0);
err_out:
    return (err);
}

int benesh_target_sub_task(struct benesh_target *tgt, struct benesh_task *task)
{
    TRACE_OUT;
    int err;

    if(!tgt || !tgt->dependent_tasks) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    if(!task) {
        ERR_OUT(BNH_EFAULT, err_out, "bad task.\n");
    }

    bnh_pvec_append(tgt->dependent_tasks, task, -1);

    return (0);
err_out:
    return (err);
}

struct benesh_directive *
benesh_target_get_directive_by_id(struct benesh_target *tgt, int dir_id)
{
    TRACE_OUT;
    size_t ndir;
    struct benesh_rule *rule;
    int err;

    if(!tgt || !tgt->rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target.\n");
    }
    CHECK_ZERO(benesh_target_get_ndir(tgt, &ndir), BNH_ESTATE, err_out,
               "could not access target.\n");
    if(dir_id < 0 || dir_id >= ndir) {
        ERR_OUT(BNH_EINVAL, err_out, "invalid directive id.\n");
    }

    rule = tgt->rule;

    return (bnh_pvec_get_by_id(rule->directives, dir_id));
err_out:
    return (NULL);
}

int benesh_directive_is_local(struct benesh_handle *bnh,
                              struct benesh_directive *dir, int *is_local)
{
    TRACE_OUT;
    struct benesh_component *my_comp;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!dir) {
        ERR_OUT(BNH_EFAULT, err_out, "bad directive.\n");
    }
    if(!is_local) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    ASSIGN_NOT_NULL(benesh_my_comp(bnh), my_comp, BNH_ESTATE, err_out,
                    "could not find my own component.\n");
    *is_local = 0;
    if(my_comp == dir->comp) {
        *is_local = 1;
    }

    return (0);
err_out:
    return (err);
}

int benesh_directive_get_type(struct benesh_directive *dir,
                              enum bnh_task_type *type)
{
    TRACE_OUT;
    int err;

    if(!dir) {
        ERR_OUT(BNH_EFAULT, err_out, "bad directive.\n");
    }
    if(!type) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    *type = dir->type;

    return (0);
err_out:
    return (err);
}