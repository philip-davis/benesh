#ifndef _BENESHP_H
#define _BENESHP_H

#include "benesh-cohort.h"
#include "benesh-tasks.h"
#include "benesh-types.h"

struct benesh_taskman *benesh_get_taskman(struct benesh_handle *bnh);

struct benesh_cohort *benesh_get_cohort(struct benesh_handle *bnh);

#endif // _BENESHP_H