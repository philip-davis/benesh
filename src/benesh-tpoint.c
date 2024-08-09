#include "benesh-tpoint.h"
#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-obj.h"
#include "benesh.h"
#include "util.h"

struct benesh_touchpoint {
    struct benesh_component *comp;
    struct bnh_pvec *vars;
    struct bnh_pvec *tgts;
    struct benesh_obj *obj;
};

struct benesh_touchpoint *benesh_tp_get_by_id(benesh_tpoints tpoints, int tp_id)
{
    TRACE_OUT;
    int err;

    if(!tpoints) {
        ERR_OUT(BNH_EFAULT, err_out, "bad touchpoint list.\n");
    }

    return (bnh_pvec_get_by_id(tpoints, tp_id));

err_out:
    return (NULL);
}

int benesh_tp_get_nvar(struct benesh_touchpoint *tpoint, size_t *nvar)
{
    TRACE_OUT;
    int err;

    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad touchpoint.\n");
    }
    *nvar = bnh_pvec_get_len(tpoint->vars);

    return (0);
err_out:
    *nvar = 0;
    return (err);
}

char *benesh_tp_to_str(struct benesh_touchpoint *tpoint, int64_t *var_map)
{
    TRACE_OUT;
    struct benesh_obj *resolve_obj;
    char *str = NULL;
    int err;

    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad touchpoint.\n");
    }

    if(var_map) {
        ASSIGN_NOT_NULL(benesh_obj_resolve(tpoint->obj, var_map), resolve_obj,
                        BNH_ENOMEM, err_out,
                        "could not resolve touchpoint object.\n");
        ASSIGN_NOT_NULL(benesh_obj_to_str(resolve_obj), str, BNH_ENOMEM,
                        err_out, "could not stringify resolved object.\n");
        CHECK_ZERO(benesh_obj_free(resolve_obj), err, err_out,
                   "failed to cleanup resolved object.\n");
    } else {
        ASSIGN_NOT_NULL(benesh_obj_to_str(tpoint->obj), str, BNH_ENOMEM,
                        err_out, "could not stringify touchpoint object.\n");
    }

    return (str);
err_out:
    if(str)
        free(str);
    return (NULL);
}

struct benesh_component *benesh_tp_get_comp(struct benesh_touchpoint *tpoint)
{
    TRACE_OUT;
    int err;

    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad touchpoint.\n");
    }

    return (tpoint->comp);
err_out:
    return (NULL);
}

int benesh_tp_handle(struct benesh_handle *bnh,
                     struct benesh_touchpoint *tpoint, int64_t *var_map)
{
    TRACE_OUT;
    int ntgt;
    char *tp_str = NULL;
    struct benesh_obj *tgt, *resolved_tgt;
    bnh_pvec_iter bi;
    int err;

    if(!bnh) {
        ERR_OUT(BNH_EFAULT, err_out, "bad benesh handle.\n");
    }
    if(!tpoint) {
        ERR_OUT(BNH_EFAULT, err_out, "bad touchpoint.\n");
    }

    if(benesh_debug_enabled()) {
        ASSIGN_NOT_NULL(benesh_tp_to_str(tpoint, var_map), tp_str, BNH_ESTATE,
                        err_out, "could not stringify announced touchpoint.");
        DEBUG_OUT("handling touchpoint %s\n", tp_str);
        free(tp_str);
        tp_str = NULL;
    }

    ntgt = bnh_pvec_get_len(tpoint->tgts);
    DEBUG_OUT("scheduling %i targets\n", ntgt);
    BNH_PVEC_FOREACH(tgt, bi, tpoint->tgts)
    {
        resolved_tgt = NULL;
        ASSIGN_NOT_NULL(benesh_obj_resolve(tgt, var_map), resolved_tgt,
                        BNH_ESTATE, err_out, "could not resolve target.\n");
        CHECK_ZERO(benesh_schedule_target(bnh, resolved_tgt), err, err_out_free,
                   "could not schedule target.\n");
        CHECK_ZERO(benesh_obj_free(resolved_tgt), err, err_out,
                   "failed to free resolved target.\n");
    }

    return (0);
err_out_free:
    if(resolved_tgt) {
        CHECK_ZERO(benesh_obj_free(resolved_tgt), err, err_out,
                   "failed to free resolved target.\n");
    }
err_out:
    if(tp_str)
        free(tp_str);
    return (err);
}