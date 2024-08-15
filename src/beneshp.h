#ifndef _BENESHP_H
#define _BENESHP_H

#include "benesh-cohort.h"
#include "benesh-obj.h"
#include "benesh-tasks.h"
#include "benesh-types.h"

struct benesh_taskman *benesh_get_taskman(struct benesh_handle *bnh);

int benesh_signal_taskman(struct benesh_handle *bnh);

struct benesh_cohort *benesh_get_cohort(struct benesh_handle *bnh);

struct benesh_component *benesh_my_comp(struct benesh_handle *bnh);

struct benesh_component *benesh_get_comp_by_id(struct benesh_handle *bnh,
                                               int id);

struct benesh_rule *benesh_get_rule_by_id(struct benesh_handle *bnh, int id);

struct benesh_touchpoint *benesh_get_tpoint_by_id(struct benesh_handle *bnh,
                                                  int id);

void benesh_sleep_til_ready(struct benesh_handle *bnh);

int benesh_my_comp_id(struct benesh_handle *bnh, int *comp_id);

int benesh_add_import_task_by_ids(struct benesh_handle *bnh, int rule_id,
                                  int directive_id, int64_t *tgt_vars);

int benesh_disconnect(struct benesh_handle *bnh, int comp_id, int *remaining);

int benesh_schedule_obj(struct benesh_handle *bnh, struct benesh_obj *obj);

int benesh_get_target_status(struct benesh_handle *bnh,
                             struct benesh_rule *rule, int64_t *var_map,
                             int *status);

int benesh_obj_to_tpoint(struct benesh_handle *bnh, struct benesh_obj *obj,
                         struct benesh_touchpoint **tpoint, int64_t **var_map);

struct benesh_target *benesh_obj_resolve_to_tgt(struct benesh_handle *bnh,
                                                struct benesh_obj *obj,
                                                int64_t *var_map);

struct bnh_pvec *benesh_prereq_targets(struct benesh_handle *bnh,
                                       struct benesh_target *tgt);

#endif // _BENESHP_H