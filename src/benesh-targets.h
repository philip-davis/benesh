#ifndef _BENESH_TARGETS_H
#define _BENESH_TARGETS_H

#include <stdlib.h>

#include "benesh-obj.h"
#include "benesh-tasks.h"
#include "benesh.h"
#include "util.h"

enum bnh_tgt_status { BNH_STAT_UNREAL, BNH_STAT_INPROGRESS, BNH_STAT_REAL };

enum bnh_dir_status {
    BNH_DIR_STAT_NOT_SCHED,
    BNH_DIR_STAT_SCHED,
    BNH_DIR_STAT_DONE
};

enum bnh_task_type;

struct benesh_directive;

struct benesh_rule;

struct benesh_target;

struct benesh_target_db;

struct benesh_task;

typedef struct bnh_pvec *benesh_rulebook;

int benesh_rule_get_nvar(struct benesh_rule *rule, size_t *nvar);

struct bnh_pvec *benesh_rule_get_directives(struct benesh_rule *rule);

int benesh_get_num_rules(benesh_rulebook rules);

struct bnh_pvec *benesh_rule_get_prereqs(struct benesh_rule *rule);

struct bnh_pvec *benesh_target_get_prereqs(struct benesh_handle *bnh,
                                           struct benesh_target *tgt);

struct bnh_pvec *benesh_target_get_directives(struct benesh_target *tgt);

int benesh_rule_get_nprereq(struct benesh_rule *rule, size_t *nprereq);

int benesh_target_get_nprereq(struct benesh_target *tgt, size_t *nprereq);

int benesh_target_get_ndir(struct benesh_target *tgt, size_t *ndir);

int benesh_rule_get_ndir(struct benesh_rule *rule, size_t *ndir);

struct benesh_rule *benesh_rule_get_by_id(benesh_rulebook rules, int rule_id);

int benesh_find_viable_rule(benesh_rulebook rules, struct benesh_obj *target,
                            struct benesh_rule **rule, int64_t **var_map);

struct benesh_target *benesh_tgt_db_lookup(struct benesh_target_db *bdb,
                                           struct benesh_rule *rule,
                                           int64_t *var_map);

int benesh_tgt_db_lock(struct benesh_target_db *bdb);

int benesh_tgt_db_unlock(struct benesh_target_db *bdb);

int benesh_target_get_status(struct benesh_target *target, int *status);

int benesh_target_dir_status(struct benesh_target *tgt, int dir_id,
                             int *status);

int benesh_target_sub_task(struct benesh_target *tgt, struct benesh_task *task);

struct benesh_directive *
benesh_target_get_directive_by_id(struct benesh_target *tgt, int dir_id);

int benesh_directive_is_local(struct benesh_handle *bnh,
                              struct benesh_directive *dir, int *is_local);

int benesh_directive_get_type(struct benesh_directive *dir,
                              enum bnh_task_type *type);

#endif // _BENESH_TARGETS_H