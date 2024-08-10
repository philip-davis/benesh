#include "benesh-logging.h"
#include "benesh.h"
#include "util.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define MAX_VAL_STR_BYTES 21

// TODO: add multiplication?
enum bnh_obj_expr_type {
    BNH_EXPR_VAL,
    BNH_EXPR_VAR,
    BNH_EXPR_ADD,
    BNH_EXPR_SUB,
    BNH_EXPR_MULT
};

struct benesh_obj_expr {
    enum bnh_obj_expr_type type;
    union {
        int64_t val;
        int var;
        struct {
            struct benesh_obj_expr *lhs;
            struct benesh_obj_expr *rhs;
        };
    };
};

enum bnh_obj_part_type { BNH_OBJ_ID, BNH_OBJ_VAL, BNH_OBJ_VAR, BNH_OBJ_EXPR };

struct benesh_obj_part {
    enum bnh_obj_part_type type;
    union {
        char *str;
        int64_t val;
        int var;
        struct benesh_obj_expr *expr;
    };
};

struct benesh_obj {
    struct bnh_pvec *parts;
    struct bnh_pvec *vars;
};

int bensh_obj_num_parts(struct benesh_obj *obj)
{
    TRACE_OUT;
    int err;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }

    return (bnh_pvec_get_len(obj->parts));
err_out:
    return (-1);
}

static char *benesh_expr_to_str(struct benesh_obj_expr *expr,
                                struct bnh_pvec *vars, int depth)
{
    TRACE_OUT;
    char *str, *val_str, *var_str, *l_str, *r_str;
    static char op_char[] = {0, 0, '+', '-', '*'};
    int str_len;
    int err;

    str = val_str = var_str = l_str = r_str = NULL;

    if(!expr) {
        ERR_OUT(BNH_EFAULT, err_out, "bad expression.\n");
    }
    switch(expr->type) {
    case BNH_EXPR_VAL:
        ASSIGN_NOT_NULL(malloc(MAX_VAL_STR_BYTES), str, BNH_ENOMEM, err_out,
                        "buffer allocation error\n");
        sprintf(str, "%" PRId64, expr->val);
        return (str);
    case BNH_EXPR_VAR:
        var_str = strdup(bnh_pvec_get_by_id(vars, expr->var));
        if(!var_str) {
            var_str = strdup("<var_stringify_err>");
        }
        return (var_str);
    case BNH_EXPR_ADD:
    case BNH_EXPR_SUB:
    case BNH_EXPR_MULT:
        ASSIGN_NOT_NULL(benesh_expr_to_str(expr->lhs, vars, depth + 1), l_str,
                        BNH_ESTATE, err_out,
                        "error resolving left-hand side of expression.\n");
        ASSIGN_NOT_NULL(benesh_expr_to_str(expr->rhs, vars, depth + 1), r_str,
                        BNH_ESTATE, err_out,
                        "error resolving right-hand side of expression.\n");
        // extra bytes for operator symbol, terminator, parentheses (if not
        // outermost expression)
        str_len = (depth ? 2 : 0) + 2 + strlen(l_str) + strlen(r_str);
        ASSIGN_NOT_NULL(malloc(str_len), str, BNH_ENOMEM, err_out,
                        "buffer allocation failure.\n");
        if(strcmp(l_str, "0") == 0 && expr->type == BNH_EXPR_SUB) {
            /*
                unwinding a kludge in parsing. -x in expressions is internally
               reprsented as 0-x.
            */
            sprintf(str, "-%s", r_str);
        } else if(depth) {
            sprintf(str, "(%s%c%s)", l_str, op_char[expr->type], r_str);
        } else {
            sprintf(str, "%s%c%s", l_str, op_char[expr->type], r_str);
        }
        free(l_str);
        free(r_str);
        return (str);
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown expression type %i\n",
                expr->type);
    }

    return (str);
err_out:
    if(str)
        free(str);
    if(val_str)
        free(val_str);
    if(var_str)
        free(var_str);
    if(l_str)
        free(l_str);
    if(r_str)
        free(r_str);
    return (strdup("<expr_stringify_err>"));
}

static char *benesh_part_to_str(struct benesh_obj_part *part,
                                struct bnh_pvec *vars)
{
    TRACE_OUT;
    char *str, *var_str, *expr_str;
    int err;

    str = var_str = expr_str = NULL;

    if(!part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object part.\n");
    }

    switch(part->type) {
    case BNH_OBJ_ID:
        return (strdup(part->str));
    case BNH_OBJ_VAL:
        ASSIGN_NOT_NULL(malloc(MAX_VAL_STR_BYTES), str, BNH_ENOMEM, err_out,
                        "buffer allocation error\n");
        sprintf(str, "%" PRId64, part->val);
        return (str);
    case BNH_OBJ_VAR:
        var_str = strdup(bnh_pvec_get_by_id(vars, part->var));
        if(!var_str) {
            var_str = strdup("<var_stringify_err>");
        }
        ASSIGN_NOT_NULL(bnh_bracket_str("%{", var_str, "}"), str, BNH_ENOMEM,
                        err_out, "buffer allocation error\n");
        free(var_str);
        return (str);
    case BNH_OBJ_EXPR:
        expr_str = benesh_expr_to_str(part->expr, vars, 0);
        if(!expr_str) {
            expr_str = strdup("<expr_stringify_err>");
        }
        ASSIGN_NOT_NULL(bnh_bracket_str("%[[]]", expr_str, "]]"), str,
                        BNH_ENOMEM, err_out, "buffer allocation error\n");
        free(expr_str);
        return (str);
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown object part type %i\n",
                part->type);
    }

err_out:
    if(str)
        free(str);
    if(var_str)
        free(var_str);
    if(expr_str)
        free(var_str);
    return (strdup("<obj_part_stringify_err>"));
}

char *benesh_obj_to_str(struct benesh_obj *obj)
{
    TRACE_OUT;
    struct benesh_obj_part *part;
    bnh_pvec_iter bi;
    char **part_strs, **part_strs_i;
    char *str;
    size_t str_len = 0;
    size_t npart;
    int i, err;

    part_strs = NULL;
    str = NULL;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }

    npart = bnh_pvec_get_len(obj->parts);
    if(npart < 0) {
        str = strdup("<obj_stringify_err>");
        return (str);
    } else if(!npart) {
        str = strdup("");
        return (str);
    }
    ASSIGN_NOT_NULL(malloc(sizeof(*part_strs) * npart), part_strs, BNH_ENOMEM,
                    err_out, "buffer allocation error.\n");
    part_strs_i = part_strs;
    BNH_PVEC_FOREACH(part, bi, obj->parts)
    {
        *part_strs_i = benesh_part_to_str(part, obj->vars);
        if(!*part_strs_i) {
            *part_strs_i = strdup("<obj_part_stringify_err>");
        }
        str_len += strlen(*part_strs_i) + 1;
        part_strs_i++;
    }
    ASSIGN_NOT_NULL(malloc(str_len), str, BNH_ENOMEM, err_out,
                    "buffer allocation error.\n");
    strcpy(str, part_strs[0]);
    free(part_strs[0]);
    for(i = 1; i < npart; i++) {
        strcat(str, ".");
        strcat(str, part_strs[i]);
        free(part_strs[i]);
    }
    free(part_strs);

    return (str);
err_out:
    if(part_strs)
        free(part_strs);
    if(str)
        free(str);
    return (strdup("<obj_stringify_err>"));
}

struct benesh_obj *benesh_obj_new(size_t npart, size_t nvar)
{
    TRACE_OUT;
    struct benesh_obj *obj = NULL;
    int err;

    ASSIGN_NOT_NULL(malloc(sizeof(*obj)), obj, BNH_ENOMEM, err_out,
                    "buffer allocation failed.\n");
    ASSIGN_NOT_NULL(bnh_pvec_new(npart), obj->parts, BNH_ENOMEM, err_out,
                    "could not create parts vector.\n");
    ASSIGN_NOT_NULL(bnh_pvec_new(nvar), obj->vars, BNH_ENOMEM, err_out,
                    "could not create variable vector.\n");

    return (obj);
err_out:
    if(obj)
        free(obj);
    return (NULL);
}

static int benesh_obj_expr_free(struct benesh_obj_expr *expr)
{
    TRACE_OUT;
    int err;

    if(expr && (expr->type >= BNH_EXPR_ADD)) {
        CHECK_ZERO(benesh_obj_expr_free(expr->lhs), err, err_out,
                   "faield to free lhs expression. Likely memory leak.\n");
        free(expr->lhs);
        expr->lhs = NULL;
        CHECK_ZERO(benesh_obj_expr_free(expr->rhs), err, err_out,
                   "faield to free rhs expression. Likely memory leak.\n");
        free(expr->rhs);
        expr->rhs = NULL;
    }

    return (0);
err_out:
    if(expr->lhs)
        free(expr->lhs);
    if(expr->rhs)
        free(expr->rhs);
    return (err);
}

static struct benesh_obj_expr *benesh_obj_expr_dup(struct benesh_obj_expr *expr)
{
    TRACE_OUT;
    struct benesh_obj_expr *new_expr = NULL;
    int err;

    if(!expr) {
        return (NULL);
    }

    ASSIGN_NOT_NULL(calloc(1, sizeof(*new_expr)), new_expr, BNH_ENOMEM, err_out,
                    "failed to allocated buffer.\n");
    new_expr->type = expr->type;
    switch(expr->type) {
    case BNH_EXPR_VAL:
        new_expr->val = expr->val;
        break;
    case BNH_EXPR_VAR:
        new_expr->var = expr->var;
        break;
    case BNH_EXPR_ADD:
    case BNH_EXPR_SUB:
    case BNH_EXPR_MULT:
        ASSIGN_NOT_NULL(benesh_obj_expr_dup(expr->lhs), new_expr->lhs,
                        BNH_ENOMEM, err_out,
                        "failed to duplicate lhs expression.\n");
        ASSIGN_NOT_NULL(benesh_obj_expr_dup(expr->rhs), new_expr->rhs,
                        BNH_ENOMEM, err_out_free,
                        "failed to duplicate rhs expression.\n");
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown expression type %i\n",
                expr->type);
    }

    return (new_expr);
err_out_free:
    CHECK_ZERO(benesh_obj_expr_free(new_expr), err, err_out,
               "failed to free expressione structures.\n");
err_out:
    if(new_expr) {
        free(new_expr);
    }
    return (NULL);
}

static struct benesh_obj_part *benesh_obj_part_dup(struct benesh_obj_part *part)
{
    TRACE_OUT;
    struct benesh_obj_part *new_part = NULL;
    int err;

    if(!part) {
        return (NULL);
    }

    ASSIGN_NOT_NULL(malloc(sizeof(*new_part)), new_part, BNH_ENOMEM, err_out,
                    "failed to allocated buffer.\n");
    new_part->type = part->type;
    switch(part->type) {
    case BNH_OBJ_ID:
        new_part->str = strdup(part->str);
        break;
    case BNH_OBJ_VAL:
        new_part->val = part->val;
        break;
    case BNH_OBJ_VAR:
        new_part->var = part->var;
        break;
    case BNH_OBJ_EXPR:
        new_part->expr = benesh_obj_expr_dup(part->expr);
        break;
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown object part type %i\n",
                part->type);
    }

    return (new_part);

err_out:
    if(new_part)
        free(new_part);
    return (NULL);
}

static int benesh_resolve_obj_expr(struct benesh_obj_expr *expr,
                                   int64_t *var_map, int64_t *val)
{
    TRACE_OUT;
    int64_t l_val, r_val;
    int err;

    if(!expr) {
        ERR_OUT(BNH_EFAULT, err_out, "bad expression.\n");
    }
    if(!var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad variable map.\n");
    }
    if(!val) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    switch(expr->type) {
    case BNH_EXPR_VAL:
        *val = expr->val;
        break;
    case BNH_EXPR_VAR:
        *val = var_map[expr->var];
        break;
    case BNH_EXPR_ADD:
    case BNH_EXPR_SUB:
    case BNH_EXPR_MULT:
        CHECK_ZERO(benesh_resolve_obj_expr(expr->lhs, var_map, &l_val), err,
                   err_out, "could not resolve left-hand expression.\n")
        CHECK_ZERO(benesh_resolve_obj_expr(expr->rhs, var_map, &r_val), err,
                   err_out, "could not resolve right-hand expression.\n")
        if(expr->type == BNH_EXPR_ADD) {
            *val = l_val + r_val;
        } else if(expr->type == BNH_EXPR_SUB) {
            *val = l_val - r_val;
        } else if(expr->type == BNH_EXPR_MULT) {
            *val = l_val * r_val;
        } else {
            ERR_OUT(BNH_ESTATE, err_out, "corruption of expr->type.\n");
        }
        break;
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown expression type %i\n",
                expr->type);
    }

    return (0);
err_out:
    return (err);
}

static int benesh_resolve_obj_part(struct benesh_obj_part *part,
                                   int64_t *var_map, int64_t *val)
{
    TRACE_OUT;
    int err;

    if(!part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad part.\n");
    }
    if(!var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad variable map.\n");
    }
    if(!val) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output pointer.\n");
    }

    if(part->type == BNH_OBJ_VAR) {
        *val = var_map[part->var];
    } else if(part->type == BNH_OBJ_EXPR) {
        CHECK_ZERO(benesh_resolve_obj_expr(part->expr, var_map, val), err,
                   err_out, "failed to resolve expression.\n");
    }

    return (0);

err_out:
    return (err);
}

static struct benesh_obj_part *benesh_obj_val_part_new(int64_t val)
{
    TRACE_OUT;
    struct benesh_obj_part *part;

    part = malloc(sizeof(*part));
    if(!part) {
        return (NULL);
    }

    part->type = BNH_OBJ_VAL;
    part->val = val;

    return (part);
}

static int benesh_obj_add_part(struct benesh_obj *obj,
                               struct benesh_obj_part *part)
{
    TRACE_OUT;
    int err;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    if(!part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad part.\n");
    }

    bnh_pvec_append(obj->parts, part, -1);

    return (0);
err_out:
    return (err);
}

static int benesh_obj_part_free(struct benesh_obj_part *part)
{
    TRACE_OUT;
    int err;

    if(!part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad part.\n");
    }

    switch(part->type) {
    case BNH_OBJ_ID:
        free(part->str);
        break;
    case BNH_OBJ_VAL:
    case BNH_OBJ_VAR:
        break;
    case BNH_OBJ_EXPR:
        CHECK_ZERO(benesh_obj_expr_free(part->expr), err, err_out,
                   "failed to free expression. Probable memory leak.\n");
        break;
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown object part type %i\n",
                part->type);
    }
    return (0);
err_out:
    return (err);
}

int benesh_obj_free(struct benesh_obj *obj)
{
    TRACE_OUT;
    struct benesh_obj_part *part;
    bnh_pvec_iter bi;
    int err;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }

    BNH_PVEC_FOREACH(part, bi, obj->parts)
    {
        CHECK_ZERO(benesh_obj_part_free(part), err, err_out,
                   "failed to free object part. Probably memory leak.\n");
        free(part);
    }
    bnh_pvec_destroy(obj->parts, 0);
    bnh_pvec_destroy(obj->vars, 1);

    return (0);
err_out:
    return (err);
}

struct benesh_obj *benesh_obj_resolve(struct benesh_obj *obj, int64_t *var_map)
{
    TRACE_OUT;
    struct benesh_obj *new_obj;
    size_t npart, nvar;
    struct benesh_obj_part *part, *new_part;
    int64_t val;
    bnh_pvec_iter bi;
    int i, err;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    if(!var_map) {
        ERR_OUT(BNH_EFAULT, err_out, "bad variable map.\n");
    }

    npart = bnh_pvec_get_len(obj->parts);
    nvar = bnh_pvec_get_len(obj->vars);
    if(npart < 0 || nvar < 0) {
        ERR_OUT(BNH_EFAULT, err_out, "invalid object.\n");
    }
    ASSIGN_NOT_NULL(benesh_obj_new(npart, 0), new_obj, err, err_out,
                    "could not allocate new object.\n");
    BNH_PVEC_FOREACH(part, bi, obj->parts)
    {
        switch(part->type) {
        case BNH_OBJ_ID:
        case BNH_OBJ_VAL:
            ASSIGN_NOT_NULL(benesh_obj_part_dup(part), new_part, BNH_ENOMEM,
                            err_out_free, "failed to copy part.\n");
            break;
        case BNH_OBJ_VAR:
        case BNH_OBJ_EXPR:
            CHECK_ZERO(benesh_resolve_obj_part(part, var_map, &val), err,
                       err_out_free, "could not resolve part.\n");
            ASSIGN_NOT_NULL(
                benesh_obj_val_part_new(val), new_part, BNH_ENOMEM,
                err_out_free,
                "failed to create new value-carrying object part.\n");
            break;
        default:
            ERR_OUT(BNH_ESTATE, err_out_free, "unknown object part type %i\n",
                    part->type);
        }
        CHECK_ZERO(benesh_obj_add_part(new_obj, new_part), err, err_out_free,
                   "failed to add new part.\n");
    }

    return (new_obj);

err_out_free:
    CHECK_ZERO(benesh_obj_free(new_obj), err, err_out,
               "failed to cleanup object structure. Probable memory leak.\n");
err_out:
    return (NULL);
}

static int benesh_expr_linear_terms(struct benesh_obj_expr *expr,
                                    int *is_var_set, int *var, int *coeff,
                                    int *constant)
{
    TRACE_OUT;
    int l_coeff, l_const, r_coeff, r_const;
    int err;

    if(!expr) {
        ERR_OUT(BNH_EFAULT, err_out, "bad expression.\n");
    }
    if(!is_var_set || !var || !coeff || !constant) {
        ERR_OUT(BNH_EFAULT, err_out, "best target output structure.\n");
    }

    switch(expr->type) {
    case BNH_EXPR_VAL:
        *constant = expr->val;
        *coeff = 0;
        break;
    case BNH_EXPR_VAR:
        if(is_var_set && expr->var != *var) {
            // two variables in one expression
            ERR_OUT(BNH_EINVAL, err_out,
                    "detected an invalid expression in a rule target during "
                    "unification. Target expressions should be linear "
                    "equations of at most one variable.\n");
        }
        *constant = 0;
        *coeff = 1;
        *var = expr->var;
        *is_var_set = 1;
        break;
    case BNH_EXPR_ADD:
    case BNH_EXPR_SUB:
    case BNH_EXPR_MULT:
        CHECK_ZERO(benesh_expr_linear_terms(expr->lhs, is_var_set, var,
                                            &l_coeff, &l_const),
                   err, err_out,
                   "could not linearize left hand size of expression.\n");
        CHECK_ZERO(benesh_expr_linear_terms(expr->rhs, is_var_set, var,
                                            &r_coeff, &r_const),
                   err, err_out,
                   "could not linearize left hand size of expression.\n");
        if(expr->type == BNH_EXPR_ADD) {
            *coeff = l_coeff + r_coeff;
            *constant = l_const + r_const;
        } else if(expr->type == BNH_EXPR_SUB) {
            *coeff = l_coeff - r_coeff;
            *constant = l_const - r_const;
        } else if(expr->type == BNH_EXPR_MULT) {
            // (ax+b)(cx+d) = acx^2 + (ad + bc)x + bd
            if(l_coeff && r_coeff) {
                // non-linear expression
                ERR_OUT(BNH_EINVAL, err_out,
                        "detected an invalid expression in a rule target "
                        "during unification. Target expressions should be "
                        "linear equations of at most one variable.\n");
            }
            *constant = l_const * r_const;
            *coeff = l_coeff * r_const + l_const * r_coeff;
        } else {
            ERR_OUT(BNH_ESTATE, err_out, "corruption of expr->type.\n");
        }
        break;
    default:
        ERR_OUT(BNH_ESTATE, err_out, "unknown expression type %i\n",
                expr->type);
    }

    return (0);
err_out:
    return (err);
}

static int benesh_unify_expr_val(struct benesh_obj_expr *expr, int64_t val,
                                 int64_t *test_var_map, int *is_var_map_set,
                                 int *is_viable)
{
    TRACE_OUT;
    int var, coeff, constant, is_var_set;
    int var_val;
    int err;

    if(!expr) {
        ERR_OUT(BNH_EFAULT, err_out, "bad expression.\n");
    }
    if(!test_var_map || !is_var_map_set || !is_viable) {
        ERR_OUT(BNH_EFAULT, err_out, "best target output structure.\n");
    }

    /*
        Solve independently as a linear equation on a single variable.
        Ideally, we'd be solving all parts simultaneously as
        a system of linear or even non-linear equations.
    */
    is_var_set = coeff = constant = 0;
    CHECK_ZERO(
        benesh_expr_linear_terms(expr, &is_var_set, &var, &coeff, &constant),
        err, err_out, "could not linearize target expression.\n");
    if(!is_var_set) {
        if(constant == val) {
            *is_viable = 1;
        }
        return (0);
    }
    if((val - constant) % coeff) {
        // only integer solutions for now
        return (0);
    }
    var_val = (val - constant) / coeff;
    if(is_var_map_set[var]) {
        if(test_var_map[var] == var_val) {
            *is_viable = 1;
        }
        return (0);
    }
    is_var_map_set[var] = 1;
    test_var_map[var] = var_val;
    *is_viable = 1;

    return (0);
err_out:
    *is_viable = 0;
    return (err);
}

static int benesh_unify_part_target(struct benesh_obj_part *part,
                                    struct benesh_obj_part *tgt_part,
                                    int64_t *test_var_map, int *is_var_map_set,
                                    int *is_viable)
{
    TRACE_OUT;
    int var;
    int is_expr_viable;
    int err;

    *is_viable = 0;

    if(!part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad part.\n");
    }
    if(!tgt_part) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target part.\n");
    }

    switch(tgt_part->type) {
    case BNH_OBJ_EXPR:
    case BNH_OBJ_VAR:
        ERR_OUT(BNH_EINVAL, err_out, "target must be fully resolved.\n");
    case BNH_OBJ_ID:
        if(part->type == BNH_OBJ_ID &&
           (strcmp(part->str, tgt_part->str) == 0)) {
            *is_viable = 1;
        }
        return (0);
    case BNH_OBJ_VAL:
        switch(part->type) {
        case BNH_OBJ_ID:
            return (0);
        case BNH_OBJ_VAL:
            if(part->val == tgt_part->val) {
                *is_viable = 1;
            }
            return (0);
        case BNH_OBJ_VAR:
            var = part->var;
            if(is_var_map_set[var]) {
                if(test_var_map[var] == tgt_part->val) {
                    *is_viable = 1;
                }
                return (0);
            }
            is_var_map_set[var] = 1;
            test_var_map[var] = tgt_part->val;
            *is_viable = 1;
            return (0);
        case BNH_OBJ_EXPR:
            CHECK_ZERO(benesh_unify_expr_val(part->expr, tgt_part->val,
                                             test_var_map, is_var_map_set,
                                             &is_expr_viable),
                       err, err_out, "unifying expression failed.\n");
        default:
            ERR_OUT(BNH_ESTATE, err_out, "unknown object part type %i\n",
                    part->type);
        }
    }

    *is_viable = 1;

    return (0);
err_out:
    *is_viable = 0;
    return (err);
}

int benesh_unify_obj_target(struct benesh_obj *obj, struct benesh_obj *target,
                            int64_t **var_map, int *is_viable)
{
    TRACE_OUT;
    int npart, nvar;
    struct benesh_obj_part *obj_part, *tgt_part;
    int64_t *test_var_map;
    int *is_var_map_set;
    int is_part_viable;
    int i, j, err;

    test_var_map = NULL;
    is_var_map_set = NULL;
    *is_viable = 0;

    if(!obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad object.\n");
    }
    if(!target) {
        ERR_OUT(BNH_EFAULT, err_out, "bad target object.\n");
    }
    if(!var_map || !is_viable) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output target.\n");
    }

    npart = bnh_pvec_get_len(obj->parts);
    if(npart != bnh_pvec_get_len(target->parts)) {
        return (0);
    }

    nvar = bnh_pvec_get_len(obj->vars);
    test_var_map = malloc(sizeof(*test_var_map) * nvar);
    is_var_map_set = calloc(sizeof(*is_var_map_set), nvar);

    for(i = 0; i < npart; i++) {
        ASSIGN_NOT_NULL(bnh_pvec_get(obj->parts, i), obj_part, BNH_ENOENT,
                        err_out, "missing object part.\n");
        ASSIGN_NOT_NULL(bnh_pvec_get(target->parts, i), tgt_part, BNH_ENOENT,
                        err_out, "missing target part.\n");
        benesh_unify_part_target(obj_part, tgt_part, test_var_map,
                                 is_var_map_set, &is_part_viable);
        if(!is_part_viable) {
            goto out_free;
        }
    }

    *is_viable = 1;
    *var_map = test_var_map;
    free(is_var_map_set);

    return (0);
out_free:
    if(test_var_map)
        free(test_var_map);
    if(is_var_map_set)
        free(is_var_map_set);
    return (0);
err_out:
    if(test_var_map)
        free(test_var_map);
    if(is_var_map_set)
        free(is_var_map_set);
    return (err);
}
