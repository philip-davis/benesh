#ifndef _BENESH_TASKS_H
#define _BENESH_TASKS_H

#include "benesh-targets.h"
#include "benesh.h"

#include <inttypes.h>

enum bnh_task_type {
    BNH_TASK_ANNOUNCE,
    BNH_TASK_ASSIGN,
    BNH_TASK_METHOD,
    BNH_TASK_IMPORT,
    BNH_TASK_PUT,
    BNH_TASK_GET,
    BNH_TASK_INJECT,
    BNH_TASK_COMPLETE_OBJ,
    BNH_TASK_START_OBJ
};

struct benesh_target;

struct benesh_taskman;

struct benesh_task;

struct benesh_target_db;

struct benesh_taskman *benesh_init_task_man();

int benesh_taskman_lock(struct benesh_taskman *btm);

int benesh_taskman_unlock(struct benesh_taskman *btm);

int benesh_taskman_signal(struct benesh_taskman *btm);

int benesh_taskman_wait(struct benesh_taskman *btm);

int benesh_taskman_enqueue(struct benesh_taskman *btm,
                           struct benesh_task *task);

int benesh_taskman_enqueue_import(struct benesh_taskman *btm,
                                  struct benesh_target *tgt, int directive_id);

int benesh_taskman_schedule_target(struct benesh_handle *bnh,
                                   struct benesh_target *tgt);

int benesh_taskman_queue_empty(struct benesh_taskman *btm, int *eout);

int benesh_taskman_run_next(struct benesh_taskman *btm,
                            struct benesh_handle *bnh);

struct benesh_target *benesh_task_get_target(struct benesh_task *task);

int benesh_task_get_dir_id(struct benesh_task *task, int *dir_id);

#endif // _BENESH_TASKS_H
