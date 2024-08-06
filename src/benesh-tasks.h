#ifndef _BENESH_TASKS_H
#define _BENESH_TASKS_H

struct benesh_taskman;

struct benesh_taskman *benesh_init_task_man();

int benesh_taskman_lock(struct benesh_taskman *btm);

int benesh_taskman_unlock(struct benesh_taskman *btm);

int benesh_taskman_signal(struct benesh_taskman *btm);

int benesh_taskman_wait(struct benesh_taskman *btm);

#endif // _BENESH_TASKS_H