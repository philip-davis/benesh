#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xc_config.h"

void xc_conf_append(struct xc_config *conf, struct xc_list_node *node)
{
    struct xc_list_node *tnode;

    if(conf->subconf) {
        tnode = conf->subconf_end;
        tnode->next = conf->subconf_end = node;

    } else {
        conf->subconf = conf->subconf_end = node;
    }
    node->next = NULL;
}

struct xc_card *xc_new_card(xc_card_t type, int ndim)
{
    struct xc_card *card = malloc(sizeof(*card));

    card->type = type;
    card->ndim = ndim;

    return (card);
}

struct xc_conf_var *xc_new_conf_var(const char *name, struct xc_base_type *base,
                                    struct xc_card *card, void *val)
{
    struct xc_conf_var *var = malloc(sizeof(*var));

    var->name = name ? strdup(name) : NULL;
    var->base = base;
    var->card = card;
    var->val = val;

    return (var);
}

struct xc_list_node *xc_list_search(struct xc_list_node *list, xc_node_t type,
                                    const char *name)
{
    while(list) {
        if(list->type == type) {
            switch(list->type) {
            case XC_NODE_VAR: {
                struct xc_conf_var *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_METHOD: {
                struct xc_conf_method *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_IFACE: {
                struct xc_iface *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_DOM: {
                struct xc_domain *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_COMP: {
                struct xc_component *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_ATTR: {
                struct xc_conf_attr *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            } break;
            case XC_NODE_DMAP: {
                struct xc_domain_map *d = list->decl;
                if(strcmp(d->name, name) == 0) {
                    return (list);
                }
            }
            case XC_NODE_STR: {
                if(strcmp(list->decl, name) == 0) {
                    return (list);
                }
            } break;
            default:
                break;
            }
        }
        list = list->next;
    }

    return (NULL);
}

void *xc_conf_get(struct xc_config *conf, xc_conf_t type, const char *name)
{
    struct xc_list_node *node;
    struct xc_component *comp;

    switch(type) {
    case XC_APP:
        node = conf->subconf;
        while(node) {
            if(node->type == XC_NODE_COMP) {
                comp = node->decl;
                if(strcmp(comp->app, name) == 0) {
                    return (comp);
                }
            }
            node = node->next;
        }
        break;
    case XC_DMAP:
        // check more specific match first
        node = xc_list_search(conf->subconf, XC_NODE_DMAP, name);
        if(!node) {
            char *comp_name = strdup(name);
            node = xc_list_search(conf->subconf, XC_NODE_DMAP,
                                  strtok(comp_name, "."));
            free(comp_name);
        }
        if(node) {
            return (node->decl);
        }
        break;
    default:
        fprintf(stderr, "WARNING: no way to search for '%s'.\n", name);
    }

    return (NULL);
}

struct xc_list_node *xc_get_rule(struct xc_config *conf, xc_conf_t type,
                                 struct xc_list_node *target,
                                 struct xc_component *comp)
{
    struct xc_list_node *node = conf->subconf;
    struct xc_list_node *result = NULL;
    struct xc_list_node *xlts = NULL;

    if(type != XC_TPRULE && type != XC_OBJ) {
        fprintf(stderr, "WARNING: tried to find invalid rule type.\n");
        return (NULL);
    }

    while(node) {
        if(node->type == XC_NODE_TPRULE && type == XC_TPRULE) {
            struct xc_tprule *rule = node->decl;

            result = malloc(sizeof(*result));
            if(rule->comp == comp &&
               xc_unify_target(rule->tpoint, target, &xlts)) {
                result->decl = rule;
                result->type = XC_NODE_TPRULE;
                break;
            }
        } else if(node->type == XC_NODE_TGTRULE && type == XC_OBJ) {
            struct xc_target *tgt = node->decl;

            result = malloc(sizeof(*result));
            if(xc_unify_target(tgt->tgtobj, target, &xlts)) {
                result->decl = tgt;
                result->type = XC_NODE_TGTRULE;
                break;
            }
        }
        node = node->next;
    }

    if(result) {
        result->next = xlts;
    }

    return (result);
}

struct xc_domain_map *xc_dmap_get(struct xc_config *conf,
                                  struct xc_component *comp,
                                  struct xc_conf_var *var)
{
    struct xc_domain_map *dmap;
    char *full_name = malloc(strlen(comp->name) + strlen(var->name) + 2);

    full_name = strdup(comp->name);
    strcat(full_name, ".");
    strcat(full_name, var->name);
    dmap = xc_conf_get(conf, XC_DMAP, full_name);
    free(full_name);

    return (dmap);
}

struct xc_conf_attr *xc_attr_get(struct xc_domain *dom, const char *key)
{
    struct xc_list_node *anode;

    while(dom) {
        anode = xc_list_search(dom->decl, XC_NODE_ATTR, key);
        if(anode) {
            return (anode->decl);
        }
        dom = dom->parent;
    }

    return (NULL);
}

int xc_obj_len(struct xc_list_node *obj)
{
    int len = 0;
    while(obj) {
        len++;
        obj = obj->next;
    }
    return (len);
}

int xc_in_list(struct xc_list_node *list, struct xc_list_node *decl)
{
    if(!decl || !list) {
        return (0);
    }
    while(list) {
        if(list->type == decl->type) {
            switch(decl->type) {
            case XC_NODE_VAR: {
                struct xc_conf_var *l = list->decl;
                struct xc_conf_var *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_METHOD: {
                struct xc_conf_method *l = list->decl;
                struct xc_conf_method *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_IFACE: {
                struct xc_iface *l = list->decl;
                struct xc_iface *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_ATTR: {
                struct xc_conf_attr *l = list->decl;
                struct xc_conf_attr *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_DOM: {
                struct xc_domain *l = list->decl;
                struct xc_domain *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_COMP: {
                struct xc_component *l = list->decl;
                struct xc_component *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_DMAP: {
                struct xc_domain_map *l = list->decl;
                struct xc_domain_map *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_TPRULE: {
                struct xc_tprule *l = list->decl;
                struct xc_tprule *d = decl->decl;
                if(strcmp(l->name, d->name) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_STR: {
                const char *l = list->decl;
                const char *d = decl->decl;
                if(strcmp(l, d) == 0) {
                    return (1);
                }
            } break;
            case XC_NODE_MSUB: {
                struct xc_msub *l = list->decl;
                struct xc_msub *d = decl->decl;
                if(l->type == d->type) {
                    return (1);
                }
            } break;
            case XC_NODE_TGTRULE: {
                struct xc_target *l = list->decl;
                struct xc_target *d = decl->decl;
                if(strcmp(xc_obj_tostr(l->tgtobj), xc_obj_tostr(d->tgtobj)) ==
                   0) {
                    return (1);
                }
            } break;
            default:
                fprintf(stderr, "WARNING: unknown node type.\n");
            }
        }
        list = list->next;
    }
    return (0);
}

const char *xc_decl_name(struct xc_list_node *decl)
{
    if(!decl) {
        return (NULL);
    }

    switch(decl->type) {
    case XC_NODE_VAR: {
        struct xc_conf_var *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_METHOD: {
        struct xc_conf_method *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_IFACE: {
        struct xc_iface *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_STR: {
        return (decl->decl);
    }
    case XC_NODE_DOM: {
        struct xc_domain *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_COMP: {
        struct xc_component *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_ATTR: {
        struct xc_conf_attr *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_TPRULE: {
        struct xc_tprule *d = decl->decl;
        return (d->name);
    }
    case XC_NODE_TGTRULE: {
        struct xc_target *d = decl->decl;
        return (xc_obj_tostr(d->tgtobj));
    }
    default:
        return (NULL);
    }
}

struct xc_list_node *xc_new_list_node(void *decl, xc_node_t type)
{
    struct xc_list_node *node = malloc(sizeof(*node));
    node->decl = decl;
    node->type = type;
    node->next = NULL;
    return (node);
}

/* DUMMY OPERATION! */
#define XC_NUM_BASE_TYPES 2
struct xc_base_type *xc_get_type(struct xc_config *conf, const char *type)
{
    static int num_types = XC_NUM_BASE_TYPES;
    static struct xc_base_type base_types[XC_NUM_BASE_TYPES] = {{"real"},
                                                                {"integer"}};
    int i;

    for(i = 0; i < num_types; i++) {
        if(strcmp(type, base_types[i].name) == 0) {
            return (&base_types[i]);
        }
    }

    return (NULL);
}

struct xc_iface *xc_get_iface(struct xc_config *conf, const char *name)
{
    struct xc_list_node *node = conf->subconf;
    struct xc_iface *iface;

    while(node) {
        if(node->type == XC_NODE_IFACE) {
            iface = node->decl;
            if(strcmp(iface->name, name) == 0) {
                return (iface);
            }
        }
        node = node->next;
    }
    return (NULL);
}

struct xc_iface *xc_new_iface(const char *name, struct xc_list_node *decl)
{
    struct xc_iface *iface = malloc(sizeof(*iface));

    iface->name = name ? strdup(name) : NULL;
    iface->decl = decl;

    return (iface);
}

struct xc_domain *xc_new_domain(const char *name, struct xc_list_node *decl)
{
    struct xc_domain *domain = malloc(sizeof(*domain));

    domain->name = name ? strdup(name) : NULL;
    domain->decl = decl;
    domain->parent = NULL;

    return (domain);
}

struct xc_ival *xc_new_ival(double min, double max, xc_ival_t min_t,
                            xc_ival_t max_t)
{
    struct xc_ival *ival = malloc(sizeof(*ival));

    ival->min = min;
    ival->max = max;
    ival->min_t = min_t;
    ival->max_t = max_t;

    return (ival);
}

struct xc_conf_attr *xc_new_attr(const char *name, void *val, xc_attrval_t type,
                                 void *param)
{
    struct xc_conf_attr *attr = malloc(sizeof(*attr));

    attr->name = name ? strdup(name) : NULL;
    attr->val = val;
    attr->val_t = type;
    attr->param = param;

    return (attr);
}

int xc_add_subconf(struct xc_config *conf, void *subconf, xc_node_t type)
{
    struct xc_list_node *node = xc_new_list_node(subconf, type);

    node->next = conf->subconf;
    conf->subconf = node;

    return (0);
}

int xc_node_tostr(struct xc_list_node *node, char **buf)
{
    char node_str[68];
    struct xc_pqexpr *pqx;
    int str_len;

    switch(node->type) {
    case XC_NODE_PQVAR:
    case XC_NODE_STR:
        str_len = strlen(node->decl) + 1;
        if(buf) {
            if(!*buf) {
                *buf = malloc(str_len + 1);
            }
            strcpy(*buf, node->decl);
        }
        break;
    case XC_NODE_PQX:
        pqx = (struct xc_pqexpr *)node->decl;
        switch(pqx->type) {
        case XC_PQ_INT:
            str_len = sprintf(node_str, "%i", *(int *)pqx->val) + 1;
            if(buf) {
                if(!*buf) {
                    *buf = malloc(str_len + 1);
                }
                strcpy(*buf, node_str);
            }
            break;
        default:
            printf("WARNING: found unimplemented string conversion for PQX.\n");
        }
        break;
    default:
        printf("WARNING: found unimplemented string conversion for node.\n");
    }

    return (str_len);
}

char *xc_obj_tostr(struct xc_list_node *obj)
{
    char *str = NULL;
    char node_str[68];
    struct xc_list_node *node = obj;
    struct xc_pqexpr *pqx;
    size_t str_len = 0;

    while(node) {
        str_len += xc_node_tostr(node, NULL);
        node = node->next;
    }

    str = malloc(str_len + 1);
    str[0] = '\0';
    node = obj;
    while(node) {
        switch(node->type) {
        case XC_NODE_PQVAR:
        case XC_NODE_STR:
            // TODO: handle PQ objects better?
            strcat(str, node->decl);
            break;
        case XC_NODE_PQX:
            pqx = (struct xc_pqexpr *)node->decl;
            switch(pqx->type) {
            case XC_PQ_INT:
                sprintf(node_str, "%i", *(int *)pqx->val);
                strcat(str, node_str);
                break;
            default:
                break;
            }
        default:
            break;
        }
        node = node->next;
        if(node) {
            strcat(str, ".");
        }
    }

    return (str);
}

struct xc_list_node *xc_strto_obj(const char *str)
{
    struct xc_list_node *obj = NULL;
    struct xc_list_node **node = &obj;
    char *tokstr = strdup(str);
    char *tok;

    tok = strtok(tokstr, ".");
    while(tok) {
        *node = malloc(sizeof(**node));
        (*node)->decl = strdup(tok);
        (*node)->type = XC_NODE_STR;
        (*node)->next = NULL;
        node = &(*node)->next;
        tok = strtok(NULL, ".");
    }

    free(tokstr);

    return (obj);
}

struct xc_domain_map *xc_new_dmap(struct xc_config *conf,
                                  struct xc_list_node *obj,
                                  struct xc_list_node *dobj)
{
    struct xc_domain_map *dmap = malloc(sizeof(*dmap));
    // struct xc_list_node *subconf = conf->subconf;
    struct xc_list_node *dnode;

    dmap->name = xc_obj_tostr(obj);
    dmap->obj = obj;

    dnode = xc_find_obj(conf->subconf, dobj, XC_NODE_DOM);
    dmap->domain = dnode ? dnode->decl : NULL;

    return (dmap);
}

struct xc_msub *xc_new_msub(struct xc_list_node *val, xc_msub_t type)
{
    struct xc_msub *msub = malloc(sizeof(*msub));

    msub->val = val;
    msub->type = type;

    return (msub);
}

struct xc_conf_method *xc_new_method(const char *name,
                                     struct xc_list_node *args,
                                     struct xc_list_node *in_var,
                                     struct xc_list_node *out_var,
                                     xc_method_t type)
{
    struct xc_conf_method *meth = malloc(sizeof(*meth));

    meth->name = name ? strdup(name) : NULL;
    meth->args = args;
    meth->in_var = in_var;
    meth->out_var = out_var;

    return (meth);
}

struct xc_list_node *xc_find_obj(struct xc_list_node *list,
                                 struct xc_list_node *obj, xc_node_t type)
{
    struct xc_list_node *node;
    struct xc_component *comp;
    struct xc_domain *dom;

    if(!obj->next) {
        // object is not nested
        return (xc_list_search(list, type, obj->decl));
    }

    switch(type) {
    case XC_NODE_VAR: // an component member, if it exists
        node = xc_list_search(list, XC_NODE_COMP, obj->decl);
        if(!node) {
            return (NULL);
        }
        comp = node->decl;
        return (xc_find_obj(comp->iface->decl, obj->next, XC_NODE_VAR));
    case XC_NODE_IFACE: // no way to nest an interface
        return (NULL);
    case XC_NODE_COMP: // no way to nest a component
        return (NULL);
    case XC_NODE_DOM: // a subdomain, if it exists
        node = xc_list_search(list, XC_NODE_DOM, obj->decl);
        if(!node) {
            return (NULL);
        }
        dom = node->decl;
        return (xc_find_obj(dom->decl, obj->next, XC_NODE_DOM));
    default:
        fprintf(stderr, "WARNING: can't do nested search for %s.\n",
                xc_type_tostr(type));
    }

    return (NULL);
}

struct xc_component *xc_new_comp(struct xc_config *conf, const char *name,
                                 const char *iface, const char *app)
{
    struct xc_component *comp = malloc(sizeof(*comp));
    struct xc_list_node *inode;

    inode = xc_list_search(conf->subconf, XC_NODE_IFACE, iface);

    comp->iface = inode ? inode->decl : NULL;
    comp->name = strdup(name);
    comp->app = strdup(app);

    return (comp);
}

struct xc_list_node **xc_get_all(struct xc_list_node *list, xc_node_t type,
                                 int *rescount)
{
    struct xc_list_node *node;
    struct xc_list_node **res;
    int count = 0;

    for(node = list; node; node = node->next) {
        if(node->type == type) {
            count++;
        }
    }

    res = malloc(count * sizeof(*res));
    *rescount = count;
    if(!count)
        return NULL;
    count = 0;
    for(node = list; node; node = node->next) {
        if(node->type == type) {
            res[count++] = node;
        }
    }

    return (res);
}

/* DUMMY OPERATION! TODO: move method stuff out to its own header*/
#define XC_NUM_NATIVE_METH 3
static struct xc_conf_method *xc_builtin_method(struct xc_config *conf,
                                                const char *mname,
                                                struct xc_list_node *args)
{
    // TODO: raze and rebuild
    static int num_meth = XC_NUM_NATIVE_METH;
    static int args_set = 0;
    static struct xc_conf_method native_meth[XC_NUM_NATIVE_METH] = {
        {"boundary", NULL, NULL, NULL, XC_METH_XFORM},
        {"linterp", NULL, NULL, NULL, XC_METH_XFORM},
        {"identity", NULL, NULL, NULL, XC_METH_XFORM}};
    int i;

    if(!args_set) {
        for(i = 0; i < num_meth; i++) {
            if((strcmp(native_meth[i].name, "boundary") == 0) ||
               (strcmp(native_meth[i].name, "identity") == 0)) {
                native_meth[i].args =
                    xc_new_list_node(strdup("obj"), XC_NODE_STR);
            } else if(strcmp(native_meth[i].name, "linterp") == 0) {
                native_meth[i].args =
                    xc_new_list_node(strdup("obj"), XC_NODE_STR);
                native_meth[i].args->next =
                    xc_new_list_node(strdup("obj"), XC_NODE_STR);
            }
        }
        args_set = 1;
    }

    for(i = 0; i < num_meth; i++) {
        if(strcmp(native_meth[i].name, mname) == 0) {
            return (&native_meth[i]);
        }
    }

    return (NULL);
}

static int xc_args_valid(struct xc_conf_method *method,
                         struct xc_list_node *args)
{
    struct xc_list_node *margs = method->args;

    if(!method) {
        return (0);
    }

    while(margs || args) {
        if(!(margs && args)) {
            return (0);
        }
        margs = margs->next;
        args = args->next;
    }

    // TODO: type checking

    return (1);
}

static int xc_minst_valid(struct xc_config *conf, struct xc_minst *minst,
                          struct xc_list_node *args)
{
    struct xc_list_node *list, *mnode;
    struct xc_component *comp;

    switch(minst->type) {
    case XC_MINST_RES:
    case XC_MINST_G:
        // We have the actual method object
        return (xc_args_valid(minst->method, args));
    case XC_MINST_PQ:
        // We don't know the actual method, so check and see if any work
        list = conf->subconf;
        while(list) {
            if(list->type == XC_NODE_COMP) {
                comp = list->decl;
                mnode = xc_list_search(comp->iface->decl, XC_NODE_METHOD,
                                       minst->meth_name);
                if(mnode && xc_args_valid(mnode->decl, args->decl)) {
                    return (1);
                }
            }
            list = list->next;
        }
        return (0);
    default:
        fprintf(stderr, "WARNING: invalid method instance type.\n");
        return (0);
    }
}

static int xc_check_params(struct xc_config *conf, struct xc_minst *minst,
                           struct xc_list_node *params)
{
    struct xc_iface *iface = minst->comp->iface;
    struct xc_vmap *vmap;
    struct xc_list_node *vvnode;
    struct xc_list_node *varnode;
    struct xc_varver *vv;

    while(params) {
        vmap = params->decl;
        if(strcmp(vmap->param, "in") == 0 || strcmp(vmap->param, "out") == 0) {
            for(vvnode = vmap->vals; vvnode; vvnode = vvnode->next) {
                vv = vvnode->decl;
                varnode =
                    xc_list_search(iface->decl, XC_NODE_VAR, vv->var_name);
                if(!varnode) {
                    return 0;
                }
                vv->var = varnode->decl;
            }
        } else {
            fprintf(stderr, "unsupported parameter '%s'\n", vmap->param);
            return 0;
        }
        params = params->next;
    }

    return 1;
}

struct xc_minst *xc_new_minst(struct xc_config *conf,
                              struct xc_list_node *cname,
                              struct xc_list_node *mname,
                              struct xc_list_node *args,
                              struct xc_list_node *params)
{
    struct xc_minst *minst = calloc(1, sizeof(*minst));

    if(cname) {
        if(xc_obj_is_pq(cname)) {
            minst->type = XC_MINST_PQ;
            fprintf(stderr, "minst->comp_name = %s, minst->meth_name = %s\n",
                    (char *)cname->decl, (char *)mname->decl);
            minst->comp_name = strdup(cname->decl);
            minst->meth_name = strdup(mname->decl);
            if(params) {
                fprintf(stderr, "WARNING: paramters not supported for "
                                "parameterized components.\n");
            }
        } else {
            struct xc_list_node *node =
                xc_list_search(conf->subconf, XC_NODE_COMP, cname->decl);
            struct xc_iface *iface;

            minst->type = XC_MINST_RES;
            minst->comp = node->decl;
            iface = minst->comp->iface;
            node = xc_list_search(iface->decl, XC_NODE_METHOD, mname->decl);
            minst->method = node->decl;
            if(params && xc_check_params(conf, minst, params)) {
                minst->params = params;
            }
        }
    } else {
        minst->type = XC_MINST_G;
        minst->comp = NULL;
        minst->method = xc_builtin_method(conf, mname->decl, args);
        if(params) {
            fprintf(stderr, "ERROR: builtin methods should not take parameters "
                            "(for now).\n");
        }
    }

    minst->args = xc_minst_valid(conf, minst, args) ? args : NULL;

    return (minst);
}

struct xc_pqexpr *xc_new_pqx(xc_pqexpr_t type, void *val,
                             struct xc_pqexpr *lside, struct xc_pqexpr *rside)
{
    struct xc_pqexpr *pqx = malloc(sizeof(*pqx));

    pqx->type = type;
    pqx->val = val;
    pqx->lside = lside;
    pqx->rside = rside;

    return (pqx);
}

struct xc_tprule *xc_new_tprule(struct xc_component *comp,
                                struct xc_list_node *tp,
                                struct xc_list_node *obj)
{
    struct xc_tprule *tprule = malloc(sizeof(*tprule));
    char *tpstr = xc_obj_tostr(tp);
    size_t name_len = strlen(tpstr) + strlen(comp->name) + 2;

    tprule->comp = comp;
    tprule->tpoint = tp;
    tprule->obj = obj;
    tprule->name = malloc(name_len);
    strcpy(tprule->name, comp->name);
    strcat(tprule->name, "@");
    strcat(tprule->name, tpstr);

    free(tpstr);

    return (tprule);
}

struct xc_expr *xc_new_expr(void *left, void *right, xc_expr_t type)
{
    struct xc_expr *expr = malloc(sizeof(*expr));

    expr->type = type;
    expr->lhs = left;
    expr->rhs = right;

    return (expr);
}

struct xc_target *xc_new_tgtrule(struct xc_list_node *tgtobj,
                                 struct xc_list_node *deps,
                                 struct xc_list_node *procedure)
{
    struct xc_target *tgt = malloc(sizeof(*tgt));
    tgt->tgtobj = tgtobj;
    tgt->deps = deps;
    tgt->procedure = procedure;

    return (tgt);
}

struct xc_varver *xc_new_varver(const char *var_name, long ver)
{
    struct xc_varver *vv = malloc(sizeof(*vv));

    vv->var_name = var_name;
    vv->var = NULL;
    vv->ver = ver;

    return (vv);
}

struct xc_vmap *xc_new_vmap(const char *param, struct xc_list_node *vals)
{
    struct xc_vmap *vmap = malloc(sizeof(*vmap));

    // only support two different params for now
    if(strcmp(param, "in") != 0 && strcmp(param, "out") != 0) {
        fprintf(stderr, "ERROR: '%s' is not a supported method parameter.\n",
                param);
        free(vmap);
        return NULL;
    }

    vmap->param = param;
    vmap->vals = vals;

    return (vmap);
}

int xc_unify_method(struct xc_list_node *list, struct xc_list_node *mobj)
{
    struct xc_component *comp;
    char *mname = mobj->next->decl;

    while(list) {
        if(list->type == XC_NODE_COMP) {
            comp = list->decl;
            if(xc_list_search(comp->iface->decl, XC_NODE_METHOD, mname)) {
                return (1);
            }
        }
        list = list->next;
    }
    return (0);
}

int xc_check_unity(struct xc_list_node *list, struct xc_list_node *pqobj)
{
    struct xc_component *comp;
    struct xc_list_node *node;

    if(!pqobj) {
        return (0);
    }

    // TODO: check target rules too...
    if(pqobj->type == XC_NODE_STR) {
        if(xc_list_search(list, XC_NODE_VAR, pqobj->decl)) {
            return (1);
        }
        node = xc_list_search(list, XC_NODE_COMP, pqobj->decl);
        comp = node ? node->decl : NULL;
        if(comp) {
            return (xc_check_unity(comp->iface->decl, pqobj->next));
        }
    } else {
        node = list;
        while(node) {
            if(node->type == XC_NODE_VAR) {
                return (1);
            } else if(node->type == XC_NODE_COMP) {
                comp = node->decl;
                return (xc_check_unity(comp->iface->decl, pqobj->next));
            }
            node = node->next;
        }
    }

    return (0);
}

/*
void apply_rule(struct xc_list_node *tgt, struct xc_list_node *xlt)
{
    if(!tgt) {
        return;
    }
    if(tgt->type == XC_NODE_PQVAR && strcmp(tgt->decl, xlt->decl) == 0) {

    }
}
*/

char *xc_apply_rules(const char *tgt, struct xc_list_node *xltrules)
{
    while(xltrules) {
        struct xc_list_node *rule = xltrules->decl;
        if(strcmp(rule->decl, tgt) == 0) {
            return (rule->next->decl);
        }
        xltrules = xltrules->next;
    }
    return (NULL);
}

struct xc_pqexpr *xc_dup_pqx(struct xc_pqexpr *pqx)
{
    if(pqx) {
        struct xc_pqexpr *newpqx = malloc(sizeof(*pqx));

        newpqx->type = pqx->type;
        switch(pqx->type) {
        case XC_PQ_INT:
            newpqx->val = malloc(sizeof(int));
            memcpy(newpqx->val, pqx->val, sizeof(int));
            break;
        case XC_PQ_REAL:
            newpqx->val = malloc(sizeof(float));
            memcpy(newpqx->val, pqx->val, sizeof(float));
            break;
        case XC_PQ_VAR:
            newpqx->val = strdup(pqx->val);
            break;
        case XC_PQ_ADD:
        case XC_PQ_SUB:
            newpqx->lside = xc_dup_pqx(pqx->lside);
            newpqx->rside = xc_dup_pqx(pqx->rside);
            break;
        default:
            fprintf(stderr, "ERROR: duplicating corrupted pq expression.\n");
            return (NULL);
        }
        return (newpqx);
    } else {
        return (NULL);
    }
}

int xc_eval_pqx(struct xc_pqexpr *pqx, struct xc_list_node *xlts, float *val)
{
    float lval, rval;
    char *eval;
    float sign = 1;

    if(!pqx) {
        return (0);
    }

    switch(pqx->type) {
    case XC_PQ_INT:
        *val = *((int *)pqx->val);
        break;
    case XC_PQ_REAL:
        *val = *((float *)pqx->val);
        break;
    case XC_PQ_VAR:
        eval = xc_apply_rules(pqx->val, xlts);
        if(eval) {
            *val = atof(eval);
            if(val == 0 && strcmp(eval, "0") != 0) {
                fprintf(stderr, "ERROR:expression terms in target names "
                                "must be numeric.\n");
                return (0);
            }
        } else {
            return (0);
        }
        break;
    case XC_PQ_SUB:
        sign = -1;
    case XC_PQ_ADD:
        if(xc_eval_pqx(pqx->lside, xlts, &lval) &&
           xc_eval_pqx(pqx->rside, xlts, &rval)) {
            *val = lval + (sign * rval);
        } else {
            return (0);
        }
        break;
    default:
        fprintf(stderr, "ERROR: invalid pq expression term.\n");
        return (0);
    }

    return (1);
}

static struct xc_list_node *xc_new_xlt(char *key, char *val)
{
    struct xc_list_node *xlt = malloc(sizeof(*xlt));
    struct xc_list_node *term = malloc(sizeof(*term));

    term->decl = key;
    term->type = XC_NODE_STR;
    term->next = malloc(sizeof(*term->next));
    term->next->decl = val;
    term->next->type = XC_NODE_STR;
    term->next->next = NULL;

    xlt->type = XC_NODE_XLTRULE;
    xlt->decl = term;
    xlt->next = NULL;

    return (xlt);
}

static void xc_aggregate_pqx(struct xc_list_node *left,
                             struct xc_list_node *right, xc_pqexpr_t type)
{
    // TODO: improve with hash maps
    struct xc_list_node *tnode;
    float sign;

    if(type == XC_PQ_ADD) {
        sign = 1;
    } else if(type == XC_PQ_SUB) {
        sign = -1;
    } else {
        fprintf(stderr, "ERROR: attempted to aggregate terms across an "
                        "unknown operator.\n");
        return;
    }

    while(right) {
        struct xc_list_node *rterm = right->decl;
        while(left) {
            struct xc_list_node *lterm = left->decl;
            if(strcmp(lterm->decl, rterm->decl) == 0) {
                float lval = atof(lterm->next->decl);
                float rval = atof(rterm->next->decl);
                free(lterm->next->decl);
                free(rterm->next->decl);
                free(rterm->next);
                free(rterm);
                asprintf((char **)&lterm->next->decl, "%f",
                         sign * (lval + rval));
                tnode = right->next;
                free(right);
                break;
            }
            if(left->next) {
                left = left->next;
            } else {
                tnode = right->next;
                right->next = NULL;
                left->next = right;
                if(type == XC_PQ_SUB) {
                    float val = atof(rterm->next->decl);
                    free(rterm->next->decl);
                    asprintf((char **)&rterm->next->decl, "%f", -1 * val);
                }
                break;
            }
        }
        right = tnode;
    }
}

static struct xc_list_node *xc_simplify_pqx(struct xc_pqexpr *pqx)
{
    struct xc_list_node *simple, *left, *right;
    char *val;

    if(!pqx) {
        return (NULL);
    }

    switch(pqx->type) {
    case XC_PQ_INT:
        asprintf(&val, "%i", *(int *)pqx->val);
        simple = xc_new_xlt(strdup("1"), val);
        break;
    case XC_PQ_REAL:
        asprintf(&val, "%f", *(float *)pqx->val);
        simple = xc_new_xlt(strdup("1"), val);
        break;
    case XC_PQ_VAR:
        simple = xc_new_xlt(strdup(pqx->val), strdup("1"));
        break;
    case XC_PQ_SUB:
    case XC_PQ_ADD:
        left = xc_simplify_pqx(pqx->lside);
        right = xc_simplify_pqx(pqx->rside);
        xc_aggregate_pqx(left, right, pqx->type);
        simple = left;
    }

    return (simple);
}

static struct xc_list_node *xc_solve_pqx(struct xc_pqexpr *tgt,
                                         const char *real,
                                         struct xc_list_node *xltrules)
{
    struct xc_list_node *rule = NULL;
    struct xc_list_node *simple, *term;
    double factor = 0;
    double cterm = 0;
    float rval = atoi(real); // rval is the "right hand side"

    if(rval == 0 && strcmp(real, "0") != 0) {
        // pqx must resolve to integer value
        return (NULL);
    }
    simple = xc_simplify_pqx(tgt);
    // A list of terms. There should be at most one variable and one constant
    while(simple) {
        term = simple->decl;
        if(strcmp(term->decl, "1") == 0) {
            cterm += atof(term->next->decl);
        } else {
            char *eval = xc_apply_rules(term->decl, xltrules);
            if(eval) {
                float evalval = atof(eval);
                if(evalval == 0 && strcmp(eval, "0") != 0) {
                    // terms in pqx must be numerical
                    return (NULL);
                }
                cterm += atof(term->next->decl) * atof(eval);
            } else if(rule) {
                // the pqx is ill-posed without more xlt rules
                return (NULL);
            } else {
                rule = malloc(sizeof(*rule));
                rule->type = XC_NODE_STR;
                rule->decl = term->decl;
                rule->next = malloc(sizeof(*rule->next));
                rule->next->type = XC_NODE_STR;
                rule->next->next = NULL;
                factor = atof(term->next->decl);
            }
        }
        simple = simple->next;
    }
    rval -= cterm;
    if(rule) {
        asprintf((char **)&rule->next->decl, "%i", (int)(rval / factor));
    } else {
        if(rval) {
            // the pqx is constant, and the equality doesn't match
            return (NULL);
        }
        rule = malloc(sizeof(*rule));
        rule->type = XC_NODE_STR;
        rule->decl = NULL;
        rule->next = NULL;
    }

    return (rule);
}

int xc_unify_target(struct xc_list_node *tgt, struct xc_list_node *real,
                    struct xc_list_node **xltrules)
{
    char *eval;
    struct xc_list_node *rule, *xlt;

    if(!tgt && !real) {
        return (1);
    }
    if(!tgt || !real) {
        return (0);
    }
    switch(tgt->type) {
    case XC_NODE_STR:
        if(strcmp(tgt->decl, real->decl) != 0) {
            return (0);
        }
        break;
    case XC_NODE_PQVAR:
        eval = xc_apply_rules(tgt->decl, *xltrules);
        if(eval && strcmp(eval, real->decl) != 0) {
            return (0);
        } else if(!eval) {
            struct xc_list_node *xlt = malloc(sizeof(*xlt));
            struct xc_list_node *var = malloc(sizeof(*var));
            struct xc_list_node *val = malloc(sizeof(*val));
            val->type = XC_NODE_STR;
            val->decl = strdup(real->decl);
            val->next = NULL;
            var->type = XC_NODE_PQVAR;
            var->decl = strdup(tgt->decl);
            var->next = val;
            xlt->type = XC_NODE_XLTRULE;
            xlt->decl = var;
            xlt->next = *xltrules;
            *xltrules = xlt;
        }
        break;
    case XC_NODE_PQX:
        rule = xc_solve_pqx(tgt->decl, real->decl, *xltrules);
        if(!rule) {
            return (0);
        } else if(rule->next) {
            xlt = malloc(sizeof(*xlt));
            xlt->decl = rule;
            xlt->type = XC_NODE_XLTRULE;
            xlt->next = *xltrules;
            *xltrules = xlt;
        }
        break;
    default:
        fprintf(stderr, "WARNING: invalid unification attempted.\n");
        return (0);
    }

    return (xc_unify_target(tgt->next, real->next, xltrules));
}

struct xc_list_node *xc_dup_obj(struct xc_list_node *obj)
{
    struct xc_list_node *newobj;

    if(!obj) {
        return (NULL);
    }
    newobj = malloc(sizeof(*newobj));
    newobj->type = obj->type;
    switch(obj->type) {
    case XC_NODE_STR:
    case XC_NODE_PQVAR:
        newobj->decl = strdup(obj->decl);
        break;
    case XC_NODE_PQX:
        newobj->decl = xc_dup_pqx(obj->decl);
        break;
    default:
        fprintf(stderr, "ERROR: trying to duplicate an unknown object part.\n");
        return (NULL);
    }
    newobj->next = xc_dup_obj(obj->next);

    return (newobj);
}

int xc_list_len(struct xc_list_node *list)
{
    int len = 0;
    while(list) {
        len++;
        list = list->next;
    }
    return (len);
}

int xc_obj_is_pq(struct xc_list_node *obj)
{
    while(obj) {
        if(obj->type != XC_NODE_STR) {
            return (1);
        }
        obj = obj->next;
    }
    return (0);
}

const char *xc_type_tostr(xc_node_t type)
{
    static const char *type_str[] = {"variable",
                                     "method",
                                     "interface",
                                     "string",
                                     "method sub declaration",
                                     "domain",
                                     "interval",
                                     "attribute",
                                     "component",
                                     "domain assignment",
                                     "partially qualified expression",
                                     "partially qualified variable",
                                     "partially qualified object",
                                     "touch point rule",
                                     "expression",
                                     "target rule",
                                     "translate rule"};

    return (type_str[type]);
}

void xc_update_parents(struct xc_domain *dom)
{
    struct xc_list_node *decl = dom ? dom->decl : NULL;

    while(decl) {
        if(decl->type == XC_NODE_DOM) {
            struct xc_domain *child = decl->decl;
            child->parent = dom;
        }
        decl = decl->next;
    }
}

void xc_free_obj(struct xc_list_node *obj)
{
    struct xc_list_node *next;
    while(obj) {
        next = obj->next;
        switch(obj->type) {
        case XC_NODE_STR:
        case XC_NODE_PQVAR:
            free(obj->decl);
            break;
        default:
            fprintf(stderr, "WARNING: trying to free object with unimplmented "
                            "node type. Memory will leak.\n");
        }
        free(obj);
        obj = next;
    }
}
