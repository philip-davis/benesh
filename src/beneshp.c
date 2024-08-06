#include "benesh-cohort.h"
#include "benesh-core-types.h"
#include "benesh-logging.h"
#include "benesh-tasks.h"

struct benesh_taskman *benesh_get_taskman(struct benesh_handle *bnh)
{
    TRACE_OUT;
    if(!bnh) {
        return (NULL);
    }
    return (bnh->btm);
}

struct benesh_cohort *benesh_get_cohort(struct benesh_handle *bnh)
{
    TRACE_OUT;
    if(!bnh) {
        return (NULL);
    }
    return (bnh->bco);
}
