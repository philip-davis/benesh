#include <stdlib.h>

#include "benesh-logging.h"
#include "benesh-targets.h"
#include "benesh.h"
#include "util.h"

struct benesh_rule {
    int id;
    int nvar;
    struct benesh_obj *target;
    // prereqs
    // directives
    int ndir;
};

int benesh_rule_get_nvar(struct benesh_rule *rule, size_t *nvar)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    *nvar = rule->nvar;

    return (0);
err_out:
    *nvar = 0;
    return (err);
}

int benesh_get_num_rules(benesh_rulebook rules)
{
    TRACE_OUT;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }
    return (bnh_pvec_get_len(rules));
err_out:
    return (-1);
}

int benesh_get_num_directives(struct benesh_rule *rule)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    return (rule->ndir);
err_out:
    return (-1);
}

struct benesh_rule *benesh_rule_get_by_id(benesh_rulebook rules, int rule_id)
{
    TRACE_OUT;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }

    return (bnh_pvec_get_by_id(rules, rule_id));

err_out:
    return (NULL);
}

int benesh_find_viable_rule(benesh_rulebook rules, struct benesh_obj *target,
                            struct benesh_rule **rule, int64_t **var_map)
{
    TRACE_OUT;
    struct benesh_rule *br;
    bnh_pvec_iter bi;
    int is_match = 0;
    int err;

    if(!rules) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rulebook.\n");
    }
    if(!rule || !var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output target.\n");
    }

    BNH_PVEC_FOREACH(br, bi, rules)
    {
        CHECK_ZERO(
            benesh_unify_obj_target(br->target, target, var_map, &is_match),
            err, err_out, "error during rule matching.\n");
        if(is_match) {
            *rule = br;
            break;
        }
    }

    return (0);
err_out:
    return (err);
}