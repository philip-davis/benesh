#ifndef _BENESH_TASKS_H
#define _BENESH_TASKS_H

#include "benesh-targets.h"
#include "benesh.h"

#include <inttypes.h>

struct benesh_taskman;

struct benesh_task;

struct benesh_taskman *benesh_init_task_man();

int benesh_taskman_lock(struct benesh_taskman *btm);

int benesh_taskman_unlock(struct benesh_taskman *btm);

int benesh_taskman_signal(struct benesh_taskman *btm);

int benesh_taskman_wait(struct benesh_taskman *btm);

int benesh_enqueue(struct benesh_taskman *btm, struct benesh_task *task);

int benesh_enqueue_import(struct benesh_taskman *btm, struct benesh_rule *rule,
                          int directive_id, int64_t *tgt_vars);

int benesh_taskman_schedule_rule(struct benesh_handle *bnh,
                                 struct benesh_taskman *btm,
                                 struct benesh_rule *rule, int64_t *var_map);

#endif // _BENESH_TASKS_H