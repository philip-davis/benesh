%{
    #define _GNU_SOURCE

    #include "xc_config.h"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include<sys/stat.h>

    #include <mpi.h>

    #define yyin xc_in
    extern FILE *xc_in;

%}

%locations

%union {
    void *ptr;
    double real;
    long val;
    char *str;
}

%left '+' NEG

%token <val> ICONST
%token <str> ID
%token <real> RCONST
%token REAL INCL FNAME OBJNAME
%token INTF IN OUT
%token DOMAIN
%token COMP DASG
%token INDENT OUTDENT NEWLINE
%token OEXPR CLEXPR
%token NEG

%type <val> ivalclose ivalopen

%type <ptr> arglist array asgexpr cardinality compdecl component
%type <ptr> dasgstmt domblock dombody domsubdecl expr exprblk intfblock
%type <ptr> intfbody ival intfsubdecl methexpr methname methodblk
%type <ptr> methoddecl methodsubdecl numlist objname objtransform pbblk
%type <ptr> pointblock pqcslist pqexpr pqlist pqobj pqpart range
%type <ptr> rhs tfrexpr tgtblock tlblock vardecl varlist varmap vartype
%type <ptr> varver varverlist vmasg vmasglist

%type <real> num

%start file

%define parse.trace
%define api.prefix {xc_}
%define api.pure full
%define parse.error verbose
%code requires { struct xc_config; }
%parse-param {struct xc_config *wf}

%{
    int yylex(XC_STYPE *lvalp, XC_LTYPE *xc_lloc);
    void xc_error(XC_LTYPE *xc_lloc, struct xc_config *wf, const char *s);
    void xc_warn(XC_LTYPE *xc_lloc, struct xc_config *wf, const char *s);
%}

%%

file :
    tlblock {
        struct xc_list_node *node = $1;
        if(xc_in_list(wf->subconf, $1)) {
            char *msg;
            asprintf(&msg, "duplicate %s: %s",xc_type_tostr(node->type),
                            xc_decl_name(node));
            xc_error(&yylloc, wf, msg);
            free(msg);
        } else if(node) {
            //node->next = wf->subconf;
            //wf->subconf = node;
            xc_conf_append(wf, node);
        }
    } file
    |
    tlblock {
        struct xc_list_node *node = $1;
        if(xc_in_list(wf->subconf, $1)) {
            char *msg;
            asprintf(&msg, "duplicate %s: %s",xc_type_tostr(node->type),
                            xc_decl_name(node));
            xc_error(&yylloc, wf, msg);
            free(msg);
        } else if(node) {
            node->next = wf->subconf;
            wf->subconf = node;
        }
    }
    ;

tlblock :
    intfblock {
        $$ = $1?xc_new_list_node($1, XC_NODE_IFACE):NULL;
    }
    |
    domblock {
        $$ = $1?xc_new_list_node($1, XC_NODE_DOM):NULL;
    }
    |
    compdecl {
        $$ = $1?xc_new_list_node($1, XC_NODE_COMP):NULL;
    }
    |
    vardecl NEWLINE {
        struct xc_conf_var *var = $1;
        if(var) {
            $$ = xc_new_list_node($1, XC_NODE_VAR);
        } else {
            $$ = NULL;
        }
    }
    |
    dasgstmt NEWLINE {
        $$ = $1?xc_new_list_node($1, XC_NODE_DMAP):NULL;
    }
    |
    pointblock {
        $$ = $1?xc_new_list_node($1, XC_NODE_TPRULE):NULL;
    }
    |
    tgtblock {
        $$ = $1?xc_new_list_node($1, XC_NODE_TGTRULE):NULL;
    }
    ;


intfblock :
    INTF ID ':' NEWLINE INDENT intfbody OUTDENT {
        struct xc_list_node *decl = $6;
        while(decl) {
            if(decl->type == XC_NODE_METHOD) {
                /*
                    Check to make sure we have member variables
                    that match everything mentioned in any in:
                    or out: lines.
                */
                struct xc_conf_method *meth = decl->decl;
                struct xc_list_node *node = meth->in_var;
                while(node) {
                    if(!xc_list_search($6, XC_NODE_VAR, node->decl)) {
                        char *msg;
                        asprintf(&msg, "%s requires '%s', which is not"
                                    "a member variable",
                                    meth->name, (char *)node->decl);
                        xc_error(&yylloc, wf, msg);
                        free(msg);
                    }
                    node = node->next;
                }
                node = meth->out_var;
                while(node) {
                    if(!xc_list_search($6, XC_NODE_VAR, node->decl)) {
                        char *msg;
                        asprintf(&msg, "%s alters '%s', which is not"
                                    " a member variable",
                                    meth->name, (char *)node->decl);
                        xc_error(&yylloc, wf, msg);
                        free(msg);
                    }
                    node = node->next;
                }
            }
            decl = decl->next;
        }
        $$ = xc_new_iface($2, $6);
    }
    ;

intfbody :
    intfsubdecl intfbody {
        struct xc_list_node *decl = $1;
        if(xc_in_list($2, decl)) {
            char *msg;
            asprintf(&msg, "ignoring duplicate %s name: %s",
                        (decl->type==XC_NODE_VAR?"variable":"method"),
                        xc_decl_name(decl));
            xc_warn(&yylloc, wf, msg);
            free(msg);
            $$ = $2;
        } else if(decl) {
            decl->next = $2;
            $$ = $1;
        } else {
            $$ = $2;
        }
    }
    |
    intfsubdecl {
        $$ = $1;
    }
    ;

intfsubdecl :
    vardecl NEWLINE {
        struct xc_conf_var *var = $1;
        if(var) {
            $$ = xc_new_list_node($1, XC_NODE_VAR);
        } else {
            $$ = NULL;
        }
    }
    |
    methoddecl {
        $$ = xc_new_list_node($1, XC_NODE_METHOD);
    }
    ;

vardecl :
    vartype ID {
        struct xc_conf_var *var = $1;
        if(var) {
            var->name = strdup($2);
            var->val = NULL;
        }
        $$ = var;
    }
    |
    vartype ID '=' RCONST {
        struct xc_conf_var *var = $1;
        if(var) {
            var->name = strdup($2);
            var->val = malloc(sizeof(double));
            *((double *)(var->val)) = $4;
        }
        $$ = var;
    }
    |
    vartype ID '=' NEG RCONST {
        struct xc_conf_var *var = $1;
        if(var) {
            var->name = strdup($2);
            var->val = malloc(sizeof(double));
            *((double *)(var->val)) = $5 * -1.0;
        }
        $$ = var;
    }
    |
    vartype ID '=' ICONST {
        struct xc_conf_var *var = $1;
        if(var) {
            var->name = strdup($2);
            var->val = malloc(sizeof(double));
            *((double *)(var->val)) = $4;
        }
        $$ = var;
    }
    |
    vartype ID '=' NEG ICONST {
        struct xc_conf_var *var = $1;
        if(var) {
            var->name = strdup($2);
            var->val = malloc(sizeof(double));
            *((double *)(var->val)) = $5 * -1;
        }
        $$ = var;
    }
    ;

vartype :
    ID cardinality {
        struct xc_base_type *type = xc_get_type(wf, $1);
        if(!type) {
            char *msg;
            asprintf(&msg, "unknown type: %s", $1);
            xc_error(&yylloc, wf, msg);
            free(msg);
            $$ = NULL;
        } else {
            $$ = xc_new_conf_var(NULL, type, $2, NULL);
        }
    }
    ;

cardinality :
    /* empty */ {
        $$ = xc_new_card(XC_CARD_SCALAR, 0);
    }
    |
    '<' ICONST '>' {
        $$ = xc_new_card(XC_CARD_CONT, $2);
    }
    |
    '{' '}' {
        $$ = xc_new_card(XC_CARD_DISC, 1);
    }
    |
    '{' ICONST '}' {
        $$ = xc_new_card(XC_CARD_DISC, $2);
    }
    ;

methoddecl :
    ID '(' arglist ')' ':' NEWLINE INDENT methodblk OUTDENT {
        struct xc_list_node *mnode = $8;
        struct xc_list_node *in_var = NULL, *out_var = NULL;
        int infound = 0, outfound = 0;
        while(mnode) {
            struct xc_msub *msub = mnode->decl;
            if(msub->type == XC_MSUB_IN) {
                if(infound) {
                    char *msg;
                    asprintf(&msg, "ignoring duplicate 'in' for '%s'", $1);
                    xc_warn(&yylloc, wf, msg);
                    free(msg);
                } else {
                    infound = 1;
                    in_var = msub->val;
                }
            } else {
                if(outfound) {
                    char *msg;
                    asprintf(&msg, "ignoring duplicate 'out' for '%s'", $1);
                    xc_warn(&yylloc, wf, msg);
                    free(msg);
                } else {
                    outfound = 1;
                    out_var = msub->val;
                }
            }
            mnode = mnode->next;
        }
        $$ = xc_new_method($1, $3, in_var, out_var, XC_METH_IFACE);
    }
    |
    ID '(' arglist ')' NEWLINE {
        $$ = xc_new_method($1, $3, NULL, NULL, XC_METH_IFACE);
    }
    ;

arglist :
    /* empty */ {
        $$ = NULL;
    }
    |
    vartype ',' arglist {
        struct xc_list_node *arg = xc_new_list_node($1, XC_NODE_VAR);
        arg->next = $3;
        $$ = arg;
    }
    |
    vartype {
        $$ = xc_new_list_node($1, XC_NODE_VAR);
    }
    ;

methodblk :
    methodsubdecl NEWLINE {
        $$ = xc_new_list_node($1, XC_NODE_MSUB);
    }
    |
    methodsubdecl NEWLINE methodblk {
        struct xc_list_node *node = xc_new_list_node($1, XC_NODE_MSUB);
        node->next = $3;
        $$ = node;
    }
    ;

methodsubdecl :
    IN ':' varlist {
        $$ = xc_new_msub($3, XC_MSUB_IN);
    }
    |
    OUT ':' varlist {
        $$ = xc_new_msub($3, XC_MSUB_OUT);
    }
    ;

varlist :
    ID {
        $$ = xc_new_list_node(strdup($1), XC_NODE_STR);
    }
    |
    ID ',' varlist {
        struct xc_list_node *name = xc_new_list_node(strdup($1), XC_NODE_STR);
        if(xc_in_list($3, name)) {
            char *msg;
            asprintf(&msg, "ignoring duplicate reference: %s", $1);
            xc_warn(&yylloc, wf, msg);
            free(msg);
            $$ = $3;
        } else {
            name->next = $3;
            $$ = name;
        }
    }
    ;

domblock :
    DOMAIN ID ':' NEWLINE INDENT dombody OUTDENT {
        struct xc_domain *dom = xc_new_domain($2, $6);
        xc_update_parents(dom);
        $$ = dom;
    }

dombody :
    domsubdecl dombody {
        struct xc_list_node *decl = $1;
        if(xc_in_list($2, decl)) {
            char *msg;
            asprintf(&msg, "ignoring duplicate %s", xc_decl_name(decl));
            xc_warn(&yylloc, wf, msg);
            free(msg);
            $$ = $2;
        } else {
            decl->next = $2;
            $$ = $1;
        }
    }
    |
    domsubdecl {
        $$ = $1;
    }
    ;

domsubdecl :
    ID ':' rhs NEWLINE {
        struct xc_conf_attr *attr = $3;
        attr->name = strdup($1);
        $$ = xc_new_list_node(attr, XC_NODE_ATTR);
    }
    |
    domblock {
        struct xc_domain *dom = $1;
        $$ = xc_new_list_node(dom, XC_NODE_DOM);
    }
    ;

rhs :
    ICONST {
        int *val = malloc(sizeof(*val));
        *val = $1;
        $$ = xc_new_attr(NULL, val, XC_ATTR_INT, NULL);
    }
    |
    NEG ICONST {
        int *val = malloc(sizeof(*val));
        *val = $2 * -1;
        $$ = xc_new_attr(NULL, val, XC_ATTR_INT, NULL);
    }
    |
    RCONST {
        double *val = malloc(sizeof(*val));
        *val = $1;
        $$ = xc_new_attr(NULL, val, XC_ATTR_REAL, NULL);
    }
    |
    NEG RCONST {
        double *val = malloc(sizeof(*val));
        *val = $2 * -1.0;
        $$ = xc_new_attr(NULL, val, XC_ATTR_REAL, NULL);
    }
    |
    range {
        $$ = xc_new_attr(NULL, $1, XC_ATTR_RANGE, NULL);
    }
    |
    ID {
        $$ = xc_new_attr(NULL, strdup($1), XC_ATTR_STR, NULL);
    }
    |
    ID ':' array {
        $$ = xc_new_attr(NULL, strdup($1), XC_ATTR_STR, $3);
    }
    |
    array {
        $$ = xc_new_attr(NULL, $1, XC_ATTR_ARR, NULL);
    }
    ;

range :
    ival '*' range {
        struct xc_list_node *node = $1;
        node->next = $3;
        $$ = node;
    }
    |
    ival {
        $$ = $1;
    }
    ;

ival :
    ivalopen num ',' num ivalclose {
        struct xc_ival *ival = xc_new_ival($2, $4, $1, $5);
        $$ = xc_new_list_node(ival, XC_NODE_IVAL);
    }
    ;

ivalopen :
    '[' {
        $$ = XC_IVAL_CLOSED;
    }
    |
    '(' {
        $$ = XC_IVAL_OPEN;
    }
    ;

ivalclose :
    ']' {
        $$ = XC_IVAL_CLOSED;
    }
    |
    ')' {
        $$ = XC_IVAL_OPEN;
    }
    ;

array:
    '{' numlist '}' {
        $$ = $2;
    }
    ;

numlist:
    /* empty */ {
        $$ = NULL;
    }
    |
    num {
        struct xc_conf_attr *attr;
        double *val = malloc(sizeof(*val));

        *val = $1;
        attr = xc_new_attr(NULL, val, XC_ATTR_REAL, NULL);
        $$ = xc_new_list_node(attr, XC_NODE_ATTR);
    }
    |
    num ',' numlist {
        struct xc_conf_attr *attr;
        double *val = malloc(sizeof(*val));
        struct xc_list_node *node;

        *val = $1;
        attr = xc_new_attr(NULL, val, XC_ATTR_REAL, NULL);
        node = xc_new_list_node(attr, XC_NODE_ATTR);
        node->next = $3;
        $$ = node;
    }
    ;

num:
    ICONST {
        $$ = $1;
    }
    |
    RCONST {
        $$ = $1;
    }
    |
    NEG num {
        $$ = $2 * -1;
    }
    ;

compdecl :
    COMP ID '(' ID ')' '[' ID ']' NEWLINE {
        struct xc_component *comp = xc_new_comp(wf, $2, $4, $7);
        if(!comp->iface) {
            char *msg;
            asprintf(&msg, "component '%s' implements unknown interface '%s'",
                        $2, $4);
            xc_error(&yylloc, wf, msg);
            free(msg);
            $$ = NULL;
        } else {
            $$ = comp;
        }
    }
    ;

dasgstmt :
    pqobj DASG objname {
        struct xc_domain_map *dmap = xc_new_dmap(wf, $1, $3);
        if(!dmap->domain) {
            char *msg;
            char *objname = xc_obj_tostr($3);
            asprintf(&msg, "%s assigned unknown domain: '%s'",
                            dmap->name, objname);
            xc_error(&yylloc, wf, msg);
            free(msg);
            free(dmap);
            free(objname);
            $$ = NULL;
        } else if(xc_obj_is_pq($1)) {
            xc_error(&yylloc, wf, "cannot assign a domain "
                                    "to a partially-qualified object");
            $$ = NULL;
        } else if(xc_find_obj(wf->subconf, $1, XC_NODE_COMP) ||
                xc_find_obj(wf->subconf, $1, XC_NODE_VAR)) {
            $$ = dmap;
        } else {
            char *msg;
            char *objname = xc_obj_tostr($1);
            asprintf(&msg, "unknown object '%s'", objname);
            xc_error(&yylloc, wf, msg);
            free(msg);
            free(dmap);
            free(objname);
            $$ = NULL;
        }
    }
    ;

objname :
    ID '.' objname {
        struct xc_list_node *name = xc_new_list_node($1, XC_NODE_STR);
        name->next = $3;
        $$ = name;
    }
    |
    ID {
        $$ = xc_new_list_node($1, XC_NODE_STR);
    }
    ;

pointblock :
    component '@' pqobj ':' NEWLINE INDENT pbblk OUTDENT {
        struct xc_list_node *cnode = $1;
        $$ = xc_new_tprule(cnode->decl, $3, $7);
    }
    |
    component '@' pqobj NEWLINE {
        struct xc_list_node *cnode = $1;
        $$ = xc_new_tprule(cnode->decl, $3, NULL);
    }
    ;

pbblk :
    pqobj NEWLINE pbblk {
        if(!xc_check_unity(wf->subconf, $1)) {
            char *msg;
            char *objname = xc_obj_tostr($1);
            asprintf(&msg, "no possible resolution of ''%s'", objname);
            xc_error(&yylloc, wf, msg);
            free(msg);
            free(objname);
            $$ = $3;
        } else {
            struct xc_list_node *node = $1;
            node->next = $3;
            $$ = node;
        }
    }
    |
    pqobj NEWLINE {
        if(!xc_check_unity(wf->subconf, $1)) {
            char *msg;
            char *objname = xc_obj_tostr($1);
            asprintf(&msg, "no possible resolution of ''%s'", objname);
            xc_error(&yylloc, wf, msg);
            free(msg);
            free(objname);
            $$ = NULL;
        } else {
            $$ = xc_new_list_node($1, XC_NODE_PQOBJ);
        }
    }
    ;

component:
    ID {
        $$ = xc_list_search(wf->subconf, XC_NODE_COMP, $1);
    }
    ;

pqobj :
    pqpart '.' pqobj {
        struct xc_list_node *node = $1;
        node->next = $3;
        $$ = node;
    }
    |
    pqpart {
        $$ = $1;
    }
    ;

pqpart :
    ID {
        $$ = xc_new_list_node(strdup($1), XC_NODE_STR);
    }
    |
    '%' '{' ID '}' {
        $$ = xc_new_list_node(strdup($3), XC_NODE_PQVAR);
    }
    |
    '%' OEXPR pqexpr CLEXPR {
        $$ = xc_new_list_node($3, XC_NODE_PQX);
    }
    |
    ICONST {
        // semantically equivalent to %[[ICONST]]
        struct xc_pqexpr *pqx;
        int *val = malloc(sizeof(*val));
        *val = $1;
        pqx = xc_new_pqx(XC_PQ_INT, val, NULL, NULL);
        $$ = xc_new_list_node(pqx, XC_NODE_PQX);
    }
    ;

pqexpr :
    ID {
        $$ = xc_new_pqx(XC_PQ_VAR, strdup($1), NULL, NULL);
    }
    |
    ICONST {
        int *val = malloc(sizeof(*val));
        *val = $1;
        $$ = xc_new_pqx(XC_PQ_INT, val, NULL, NULL);
    }
    |
    RCONST {
        double *val = malloc(sizeof(*val));
        *val = $1;
        $$ = xc_new_pqx(XC_PQ_REAL, val, NULL, NULL);
    }
    |
    pqexpr '+' pqexpr {
        $$ = xc_new_pqx(XC_PQ_ADD, NULL, $1, $3);
    }
    |
    pqexpr NEG pqexpr {
        $$ = xc_new_pqx(XC_PQ_SUB,NULL, $1, $3);
    }
    |
    '(' pqexpr ')' {
        $$ = $2;
    }
    |
    NEG pqexpr {
        struct xc_pqexpr *zero;
        int *val = malloc(sizeof(*val));
        *val = 0;
        zero = xc_new_pqx(XC_PQ_INT, val, NULL, NULL);
        $$ = xc_new_pqx(XC_PQ_SUB, NULL, zero, $2);
    }
    ;

tgtblock :
    pqobj ':' pqlist NEWLINE INDENT exprblk OUTDENT {
        $$ = xc_new_tgtrule($1, $3, $6);
    }
    |
    pqobj ':' pqlist NEWLINE {
        $$ = xc_new_tgtrule($1, $3, NULL);
    }
    ;

pqlist :
    /* empty */ {
        $$ = NULL;
    }
    |
    pqobj pqlist {
        struct xc_list_node *pqobj = xc_new_list_node($1, XC_NODE_PQOBJ);
        pqobj->next = $2;
        $$ = pqobj;
    }
    ;

pqcslist :
    /* empty */ {
        $$ = NULL;
    }
    |
    pqobj {
        $$ = xc_new_list_node($1, XC_NODE_PQOBJ);
    }
    |
    pqobj ',' pqcslist {
        struct xc_list_node *pqobj = xc_new_list_node($1, XC_NODE_PQOBJ);
        pqobj->next = $3;
        $$ = pqobj;
    }
    ;

exprblk :
    expr NEWLINE exprblk {
        struct xc_list_node *node = $1;
        node->next = $3;
        $$ = node;
    }
    |
    expr NEWLINE {
        $$ = $1;
    }
    ;

expr :
    methexpr {
        struct xc_expr *expr = xc_new_expr($1, NULL, XC_EXPR_METHOD);
        $$ = xc_new_list_node(expr, XC_NODE_EXPR);
    }
    |
    asgexpr {
        $$ = xc_new_list_node($1, XC_NODE_EXPR);
    }
    |
    tfrexpr {
        $$ = xc_new_list_node($1, XC_NODE_EXPR);
    }
    ;

methexpr :
    // BUG: arguments can be real
    methname '(' pqcslist ')' varmap {
        struct xc_list_node *cmname;
        struct xc_minst *minst = NULL;

        cmname = $1;
        if(cmname) {
            if(cmname->next) {
                minst = xc_new_minst(wf, cmname, cmname->next, $3, $5);
            } else {
                minst = xc_new_minst(wf, NULL, cmname, $3, $5);
            }
        }
        //TODO check whether the arguments actually exist...
        if(minst) {
            if($3 && !minst->args) {
                //xc_new_minst leaves minst->args NULL if something went wrong.
                xc_error(&yylloc, wf, "invalid argument list");
                $$ = NULL;
            } else {
                $$ = minst;
            }
        } else {
            $$ = NULL;
        }

    }
    ;

varmap :
    ':' vmasglist {
        $$ = $2;
    }
    | {
        $$ = NULL;
    }
    ; 

vmasglist :
    vmasg ';' vmasglist {
        struct xc_list_node *node = xc_new_list_node($1, XC_NODE_VMAP);
        node->next = $3;
        $$ = node;
    }
    |
    vmasg {
        $$ = xc_new_list_node($1, XC_NODE_VMAP);
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;  
    
vmasg :
    IN '=' varverlist {
        $$ = xc_new_vmap("in", $3);
    }    
    |
    OUT '=' varverlist {
        $$ = xc_new_vmap("out", $3);
    }
    |
    ID '=' varverlist {
        // will generate an error
        $$ = xc_new_vmap($1, $3);
    }
    ;

varverlist :
    varver ',' varverlist {
        struct xc_list_node *node = xc_new_list_node($1, XC_NODE_VARVER);
        node->next = $3;
        $$ = node;
    }
    |
    varver {
        $$ = xc_new_list_node($1, XC_NODE_VARVER);    
    }
    | /* empty */ {
        $$ = NULL;
    }
    ;

varver :
    ID '.' pqobj {
        $$ = xc_new_varver($1, $3);
    }
    ;

asgexpr :
    objtransform '=' pqexpr {
        $$ = xc_new_expr($1, $3, XC_EXPR_ASG);
    }
    ;

tfrexpr :
    objtransform '<' objtransform {
        $$ = xc_new_expr($1, $3, XC_EXPR_XFR);
    }
    ;

objtransform:
    methexpr {
        $$ = $1;
    }
    |
    pqobj {
        /* identity transform - cute, but fits semantically. */
        struct xc_list_node *ident =
                    xc_new_list_node(strdup("identity"), XC_NODE_STR);
        $$ = xc_new_minst(wf, NULL, ident,
                            xc_new_list_node($1, XC_NODE_PQOBJ), NULL);
    }
    ;

methname :
    ID {
        $$ = xc_new_list_node($1, XC_NODE_STR);
    }
    |
    pqpart '.' ID {
        struct xc_list_node *comp_name = $1;

        if(comp_name->type == XC_NODE_PQX) {
            xc_error(&yylloc, wf, "variable expressions cannot"
                                    " resolve components");
            $$ = NULL;
        } else if(comp_name->type == XC_NODE_PQVAR){
            comp_name->next = xc_new_list_node($3, XC_NODE_STR);
            if(!xc_unify_method(wf->subconf, comp_name)) {
                char *msg;
                asprintf(&msg, "no components provide %s", $3);
                xc_error(&yylloc, wf, msg);
                free(msg);
                $$ = NULL;
            } else {
                $$ = comp_name;
            }
        } else {
            struct xc_list_node *node = xc_list_search(wf->subconf,
                                            XC_NODE_COMP, comp_name->decl);
            struct xc_component *comp = node?node->decl:NULL;
            comp_name->next =  xc_new_list_node($3, XC_NODE_STR);
            if(!node) {
                char *msg;
                asprintf(&msg, "no component `%s`", (char *)comp_name->decl);
                xc_error(&yylloc, wf, msg);
                free(msg);
                $$ = NULL;
            } else if(!xc_list_search(comp->iface->decl,
                                XC_NODE_METHOD, $3)) {
                char *msg;
                asprintf(&msg, "component %s does not provide method '%s'",
                                    (char *)comp_name->decl, $3);
                xc_error(&yylloc, wf, msg);
                free(msg);
                $$ = NULL;
            } else {
                $$ = comp_name;
            }
        }
    }
    ;

%%

void xc_error(XC_LTYPE *xc_lloc, struct xc_config *wf, const char *s)
{
    fprintf(stderr, "ERROR:%s:%i: %s\n", wf->fname, xc_lloc->first_line, s);
}

void xc_warn(XC_LTYPE *xc_lloc, struct xc_config *wf, const char *s)
{
    fprintf(stderr, "WARNING:%s:%i: %s\n", wf->fname, xc_lloc->first_line, s);
}

struct xc_config *xc_fparse(const char *fname, MPI_Comm comm)
{
    struct xc_config *wf = calloc(1, sizeof(*wf));
    FILE *conf_file;
    char *fbuf;
    struct stat st;
    int rank;

    MPI_Comm_rank(comm, &rank);

    wf->fname = fname;
    if(!rank) {
        stat(fname, &st);
    }
    MPI_Bcast(&st.st_size, sizeof(st.st_size), MPI_BYTE, 0, comm);
    fbuf = malloc(st.st_size);
    if(!rank) {
        conf_file = fopen(fname, "rb");
        if(!conf_file) {
            perror("could not open configuration file");
            return(NULL);
        }
        fread(fbuf, 1, st.st_size, conf_file);
    }
    MPI_Bcast(fbuf, st.st_size, MPI_BYTE, 0, comm);
    xc_in = fmemopen(fbuf, st.st_size, "r");
    xc_parse(wf);

    free(fbuf);

    return(wf);
}
