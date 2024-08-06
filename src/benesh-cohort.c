#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh.h"
#include "util.h"
#include <stdlib.h>

struct benesh_cohort {
    struct bnh_pvec *cvec;
    size_t conn_count;
};

struct benesh_component {
    int connected;
    int id;
    char *app;
    char *name;
    int size;
    int isme;
};

int benesh_disconnect(struct benesh_cohort *bco, int comp_id)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    BNH_PVEC_FOREACH(comp, bi, bco->cvec)
    {
        if(comp->id == comp_id) {
            if(comp->connected) {
                comp->connected = 0;
            } else {
                WARN_OUT("component %i is not connected.\n", comp_id);
            }
            return (0);
        }
    }

    ERR_OUT(BNH_ESRCH, err_out, "no component found with id %i\n", comp_id);
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

    BNH_PVEC_FOREACH(comp, bi, bco->cvec)
    {
        if(comp->connected)
            (*count)++;
    }

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

int benesh_my_comp_id(struct benesh_cohort *bco, int *comp_id)
{
    TRACE_OUT;
    bnh_pvec_iter bi;
    struct benesh_component *comp;
    int err;

    if(!bco) {
        ERR_OUT(BNH_EFAULT, err_out, "bad cohort.\n");
    }

    BNH_PVEC_FOREACH(comp, bi, bco->cvec)
    {
        if(comp->isme) {
            *comp_id = comp->id;
            return (0);
        }
    }

    *comp_id = BNH_COMP_NULL;
    ERR_OUT(BNH_ESTATE, err_out, "could not find my own component.\n")
err_out:
    return (err);
}