#include <stdlib.h>

#include "benesh-logging.h"
#include "benesh-targets.h"
#include "benesh.h"
#include "util.h"

struct benesh_target {
    int id;
};

struct benesh_rule {
    int id;
    int nvar;
    struct benesh_target target;
    // dependencies
    // subrules
    int nsubr;
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
    return (benesh_pvec_get_len(rules));
err_out:
    return (-1);
}

int benesh_get_num_subrules(struct benesh_rule *rule)
{
    TRACE_OUT;
    int err;

    if(!rule) {
        ERR_OUT(BNH_EFAULT, err_out, "bad rule.\n");
    }
    return (rule->nsubr);
err_out:
    return (-1);
}