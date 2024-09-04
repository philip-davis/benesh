#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh.h"
#include "util.h"

#include <abt.h>
#include <stdatomic.h>
#include <stdlib.h>

struct benesh_cohort {
    struct bnh_pvec *cvec;
    ABT_mutex mtx;
};

struct benesh_component {
    atomic_int connected;
    int id;
    char *app;
    char *name;
    int size;
    int isme;
};

int benesh_comp_disconnect(struct benesh_cohort *bco, int comp_id)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    comp = bnh_pvec_get_by_id(bco->cvec, comp_id);
    if(!comp) {
        ERR_OUT(BNH_ESRCH, err_out, "no component found with id %i\n", comp_id);
    }
    if(comp->connected) {
        comp->connected = 0;
    } else {
        WARN_OUT("component %i is not connected.\n", comp_id);
    }

    return (0);

err_out:
    return (err);
}

int benesh_connected_count(struct benesh_cohort *bco, int *count)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    int err;

    *count = 0;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    CHECK_ZERO(ABT_mutex_lock(bco->mtx), BNH_EABT, err_out,
               "failed to lock cohort.\n");
    BNH_PVEC_FOREACH(comp, bi, bco->cvec)
    {
        if(comp->connected)
            (*count)++;
    }
    CHECK_ZERO(ABT_mutex_unlock(bco->mtx), BNH_EABT, err_out,
               "failed to unlock cohort. Possible deadlock.\n");

    return (0);
err_out:
    return (err);
}

struct bnh_pvec *benesh_get_components(struct benesh_cohort *bco)
{
    TRACE_OUT;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    return (bco->cvec);
err_out:
    return (NULL);
}

struct benesh_component *benesh_comp_get_by_id(struct benesh_cohort *bco,
                                               int comp_id)
{
    TRACE_OUT;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    return (bnh_pvec_get_by_id(bco->cvec, comp_id));

err_out:
    return (NULL);
}

int benesh_get_component_count(struct benesh_cohort *bco)
{
    TRACE_OUT;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    return (bnh_pvec_get_len(bco->cvec));
err_out:
    return (-1);
}

int benesh_comp_is_me(struct benesh_component *comp, int *flag)
{
    TRACE_OUT;
    int err;

    if(!comp) {
        ERR_OUT(BNH_EFAULT, err_out, "bad component.\n");
    }
    return (comp->isme);

err_out:
    return (err);
}

char *benesh_comp_name(struct benesh_component *comp)
{
    TRACE_OUT;
    int err;

    if(!comp) {
        ERR_OUT(BNH_EFAULT, err_out, "bad component.\n");
    }
    return (comp->name);

err_out:
    return (NULL);
}

int benesh_comp_id(struct benesh_component *comp, int *id)
{
    TRACE_OUT;
    int err;

    if(!comp) {
        ERR_OUT(BNH_EFAULT, err_out, "bad component.\n");
    }
    *id = comp->id;
    return (0);

err_out:
    *id = -1;
    return (err);
}

int benesh_comp_any(struct benesh_cohort *bco, int *eout)
{
    TRACE_OUT;
    int count;
    int err;

    *eout = 0;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh cohort.\n");
    }
    if(!eout) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    CHECK_ZERO(benesh_connected_count(bco, &count), err, err_out,
               "could not access cohort.\n");

    if(count) {
        return (1);
    }
    return (0);
err_out:
    *eout = err;
    return (0);
}