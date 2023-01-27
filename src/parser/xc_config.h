#ifndef _XC_CONFIG_H
#define _XC_CONFIG_H

#include <mpi.h>

typedef enum xc_node_t {
    XC_NODE_VAR = 0,
    XC_NODE_METHOD,
    XC_NODE_IFACE,
    XC_NODE_STR,
    XC_NODE_MSUB,
    XC_NODE_DOM,
    XC_NODE_IVAL,
    XC_NODE_ATTR,
    XC_NODE_COMP,
    XC_NODE_DMAP,
    XC_NODE_PQX,
    XC_NODE_PQVAR,
    XC_NODE_PQOBJ,
    XC_NODE_TPRULE,
    XC_NODE_EXPR,
    XC_NODE_TGTRULE,
    XC_NODE_XLTRULE,
    XC_NODE_VARVER,
    XC_NODE_VMAP
} xc_node_t;

typedef enum xc_conf_t { XC_APP, XC_DMAP, XC_TPRULE, XC_OBJ } xc_conf_t;

struct xc_list_node {
    void *decl;
    xc_node_t type;
    struct xc_list_node *next;
};

struct xc_config {
    const char *fname;
    struct xc_list_node *subconf;
    struct xc_list_node *subconf_end;
};

typedef enum xc_card_t { XC_CARD_SCALAR, XC_CARD_DISC, XC_CARD_CONT } xc_card_t;

struct xc_card {
    xc_card_t type;
    int ndim;
};

struct xc_base_type {
    const char *name;
};

struct xc_conf_var {
    char *name;
    struct xc_base_type *base;
    struct xc_card *card;
    void *val;
};

// method sub-declaration type
typedef enum xc_msub_t { XC_MSUB_IN, XC_MSUB_OUT } xc_msub_t;

typedef enum xc_method_t { XC_METH, XC_METH_IFACE, XC_METH_XFORM } xc_method_t;

struct xc_conf_method {
    char *name;
    struct xc_list_node *args;
    struct xc_list_node *in_var;
    struct xc_list_node *out_var;
    xc_method_t type;
};

struct xc_msub {
    xc_msub_t type;
    struct xc_list_node *val;
};

struct xc_iface {
    char *name;
    struct xc_list_node *decl;
};

struct xc_domain {
    char *name;
    struct xc_list_node *decl;
    struct xc_domain *parent;
};

typedef enum xc_ival_t { XC_IVAL_OPEN, XC_IVAL_CLOSED } xc_ival_t;

struct xc_ival {
    xc_ival_t min_t, max_t;
    union {
        double min;
        int idmin;
    };
    union {
        double max;
        int idmax;
    };
};

typedef enum xc_attrval_t {
    XC_ATTR_REAL,
    XC_ATTR_INT,
    XC_ATTR_RANGE,
    XC_ATTR_STR,
    XC_ATTR_ARR,
    XC_ATTR_IDRANGE
} xc_attrval_t;

struct xc_conf_attr {
    char *name;
    void *val;
    xc_attrval_t val_t;
    void *param;
};

struct xc_component {
    char *name;
    struct xc_iface *iface;
    char *app;
};

struct xc_domain_map {
    char *name;
    struct xc_list_node *obj;
    struct xc_domain *domain;
    struct xc_list_node *attrs;
};

typedef enum xc_dmap_attr_t {
    XC_DMAP_ATTR_STR
} xc_dmap_attr_t;

struct xc_dmap_attr {
    void *val;
    xc_dmap_attr_t type;
};

typedef enum xc_pqexpr_t {
    XC_PQ_INT,
    XC_PQ_REAL,
    XC_PQ_ADD,
    XC_PQ_SUB,
    XC_PQ_VAR
} xc_pqexpr_t;

struct xc_pqexpr {
    xc_pqexpr_t type;
    void *val;
    struct xc_pqexpr *lside;
    struct xc_pqexpr *rside;
};

// touch point rule
struct xc_tprule {
    char *name;
    struct xc_component *comp;
    struct xc_list_node *tpoint;
    struct xc_list_node *obj;
};

// method instance type
typedef enum xc_minst_t {
    XC_MINST_RES, // resolved
    XC_MINST_PQ,  // partially-qualified
    XC_MINST_G    // global (transform, etc...)
} xc_minst_t;

// method instance
struct xc_minst {
    xc_minst_t type;
    union {
        struct xc_component *comp;
        char *comp_name;
    };
    union {
        struct xc_conf_method *method;
        char *meth_name;
    };
    struct xc_list_node *args;
    struct xc_list_node *params;
};

typedef enum xc_expr_t { XC_EXPR_METHOD, XC_EXPR_ASG, XC_EXPR_XFR, XC_EXPR_MXFR } xc_expr_t;

struct xc_expr {
    xc_expr_t type;
    union {
        struct xc_minst *minst;
        struct xc_minst *lhs;
        struct xc_minst *tgt;
    };
    union {
        struct xc_pqexpr *rhs;
        struct xc_minst *src;
        struct xc_obj_fusion *msrc;
    };
};

struct xc_target {
    struct xc_list_node *tgtobj;
    struct xc_list_node *deps;
    struct xc_list_node *procedure;
};

struct xc_varver {
    const char *var_name;
    struct xc_conf_var *var;
    struct xc_list_node *ver;
};

struct xc_vmap {
    const char *param;
    struct xc_list_node *vals;
};

struct xc_obj_fusion {
    struct xc_list_node *first;
    struct xc_list_node *second;    
};

void xc_conf_append(struct xc_config *conf, struct xc_list_node *node);

struct xc_base_type *xc_get_type(struct xc_config *conf, const char *type);

struct xc_conf_attr *xc_new_attr(const char *name, void *val, xc_attrval_t type,
                                 void *param);
struct xc_card *xc_new_card(xc_card_t type, int ndim);
struct xc_component *xc_new_comp(struct xc_config *conf, const char *name,
                                 const char *iface, const char *app);
struct xc_conf_var *xc_new_conf_var(const char *name, struct xc_base_type *base,
                                    struct xc_card *card, void *val);
struct xc_domain_map *xc_new_dmap(struct xc_config *conf,
                                  struct xc_list_node *obj,
                                  struct xc_list_node *dobj,
                                  struct xc_list_node *attrs);
struct xc_dmap_attr *xc_new_dmap_attr(xc_dmap_attr_t type, void *val);
struct xc_domain *xc_new_domain(const char *name, struct xc_list_node *decl);
struct xc_expr *xc_new_expr(void *left, void *right, xc_expr_t type);
struct xc_iface *xc_new_iface(const char *name, struct xc_list_node *decl);
struct xc_ival *xc_new_rival(double min, double max, xc_ival_t min_t,
                            xc_ival_t max_t);
struct xc_ival *xc_new_idival(int idmin, int idmax, xc_ival_t min_t, xc_ival_t max_t);
struct xc_list_node *xc_new_list_node(void *decl, xc_node_t type);
struct xc_conf_method *xc_new_method(const char *name,
                                     struct xc_list_node *args,
                                     struct xc_list_node *in_var,
                                     struct xc_list_node *out_var,
                                     xc_method_t type);
struct xc_minst *xc_new_minst(struct xc_config *conf,
                              struct xc_list_node *cname,
                              struct xc_list_node *mname,
                              struct xc_list_node *args,
                              struct xc_list_node *params);
struct xc_msub *xc_new_msub(struct xc_list_node *val, xc_msub_t type);
struct xc_pqexpr *xc_new_pqx(xc_pqexpr_t type, void *val,
                             struct xc_pqexpr *lside, struct xc_pqexpr *rside);
struct xc_target *xc_new_tgtrule(struct xc_list_node *tgtobj,
                                 struct xc_list_node *deps,
                                 struct xc_list_node *procedure);
struct xc_tprule *xc_new_tprule(struct xc_component *comp,
                                struct xc_list_node *tp,
                                struct xc_list_node *obj);
struct xc_varver *xc_new_varver(const char *var_name, struct xc_list_node *ver);
struct xc_vmap *xc_new_vmap(const char *param, struct xc_list_node *vals);
struct xc_obj_fusion *xc_new_obj_fusion(struct xc_minst *first, struct xc_minst *second);

int xc_unify_method(struct xc_list_node *list, struct xc_list_node *mobj);
int xc_unify_target(struct xc_list_node *tgt, struct xc_list_node *real,
                    struct xc_list_node **xltrules);
int xc_check_unity(struct xc_list_node *list, struct xc_list_node *pqobj);
int xc_obj_is_pq(struct xc_list_node *obj);
int xc_node_tostr(struct xc_list_node *node, char **buf);
char *xc_obj_tostr(struct xc_list_node *obj);
struct xc_list_node *xc_strto_obj(const char *str);
struct xc_list_node *xc_dup_obj(struct xc_list_node *obj);
void xc_free_obj(struct xc_list_node *obj);

int xc_obj_len(struct xc_list_node *obj);
int xc_in_list(struct xc_list_node *list, struct xc_list_node *decl);
struct xc_list_node *xc_list_search(struct xc_list_node *list, xc_node_t type,
                                    const char *name);
void *xc_conf_get(struct xc_config *conf, xc_conf_t type, const char *name);
struct xc_list_node *xc_get_rule(struct xc_config *conf, xc_conf_t type,
                                 struct xc_list_node *target,
                                 struct xc_component *comp);
struct xc_conf_attr *xc_attr_get(struct xc_domain *dom, const char *key);
struct xc_domain_map *xc_dmap_get(struct xc_config *conf,
                                  struct xc_component *comp,
                                  struct xc_conf_var *var);
const char *xc_decl_name(struct xc_list_node *decl);
struct xc_list_node *xc_find_obj(struct xc_list_node *list,
                                 struct xc_list_node *obj, xc_node_t type);
struct xc_list_node **xc_get_all(struct xc_list_node *list, xc_node_t type,
                                 int *rule_count);
const char *xc_type_tostr(xc_node_t type);
char *xc_apply_rules(const char *tgt, struct xc_list_node *xltrules);
int xc_eval_pqx(struct xc_pqexpr *pqx, struct xc_list_node *xlts, float *val);

struct xc_iface *xc_get_iface(struct xc_config *wf, const char *name);

void xc_update_parents(struct xc_domain *dom);

int xc_add_subconf(struct xc_config *wf, void *subconf, xc_node_t type);

struct xc_config *xc_fparse(const char *fname, MPI_Comm comm);

#endif
