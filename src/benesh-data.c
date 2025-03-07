#include "benesh-cohort.h"
#include "benesh-logging.h"
#include "benesh-obj.h"
#include "benesh.h"
#include "util.h"

enum bnh_var_shape {
    BNH_VAR_CONST,
    BNH_VAR_SCALAR,
    BNH_VAR_LIST,
    BNH_VAR_GRID
};

enum bnh_var_scope { BNH_SCOPE_LOCAL, BNH_SCOPE_IFACE, BNH_SCOPE_GLOBAL };

struct benesh_var_type {
    enum bnh_var_shape shape;
    enum benesh_datatype datatype;
    enum bnh_order order;
};

struct benesh_var_inst {
    struct benesh_obj *id;
    union {
        size_t len;
        struct {
            int ndim;
            int64_t *dims;
        };
    };
    union {
        long ival;
        double rval;
        void *buffer;
    };
};

struct benesh_var {
    char *name;
    enum bnh_var_scope scope;
    struct benesh_var_type type;
    union {
        struct benesh_component *comp;
        struct benesh_var_inst *cur_version;
    };
    struct benesh_obj_db *versions;
};

struct benesh_datum {
    enum benesh_datatype datatype;
    union {
        long ival;
        double rval;
    };
};

static struct benesh_var_inst *benesh_var_find_inst(struct benesh_var *var,
                                                    struct benesh_obj *ver_obj)
{
    TRACE_OUT;
    struct benesh_var_inst *inst;
    int err;

    if(!var) {
        ERR_OUT(BNH_EFAULT, err_out, "bad variable.\n");
    }
    if(!ver_obj) {
        ERR_OUT(BNH_EFAULT, err_out, "bad version object.\n");
    }

err_out:
    return (NULL;);
}

static int benesh_var_ver_to_datum(struct benesh_var *var,
                                   struct benesh_obj *ver_obj,
                                   struct benesh_datum *datum)
{
    TRACE_OUT;
    struct bensh_var_inst *inst;
    int err;

    if(!var) {
        ERR_OUT(BNH_EFAULT, err_out, "bad variable.\n");
    }
    if(!datum) {
        ERR_OUT(BNH_EFAULT, err_out, "bad output structure.\n");
    }

    switch(var->type.shape) {
    case BNH_VAR_CONST:
        if(ver_obj) {
            ERR_OUT(BNH_EINVAL, err_out, "constants cannot be versioned.\n");
        }
    case BNH_VAR_SCALAR:
        if(ver_obj) {
            inst = benesh_var_find_inst(var, ver_obj);
        } else {
            inst = var->cur_version;
        }
    }

    return (0);
err_out:
    return (err);
}

int benesh_var_assign(struct benesh_var *from_var,
                      struct benesh_obj *from_version,
                      struct benesh_var *to_var, struct benesh_obj *to_version)
{
    TRACE_OUT;
    struct benesh_datum datum;
    int is_resolved;
    int err;

    if(!from_var) {
        ERR_OUT(BNH_EFAULT, err_out, "bad from variable.\n");
    }
    if(!to_var) {
        ERR_OUT(BNH_EFAULT, err_out, "bad to variable.\n");
    }

    if(from_version) {
        CHECK_ZERO(benesh_obj_fully_resolved(from_version, &is_resolved), err,
                   err_out, "could not inspect from version.\n")
        if(!is_resolved) {
            ERR_OUT(BNH_EINVAL, err_out, "from version not fully resolved.\n");
        }
    }
    if(to_version) {
        CHECK_ZERO(benesh_obj_fully_resolved(to_version, &is_resolved), err,
                   err_out, "could not inspect to version.\n")
        if(!is_resolved) {
            ERR_OUT(BNH_EINVAL, err_out, "to version not fully resolved.\n");
        }
    }

    switch(to_var->type.shape) {
    case BNH_VAR_CONST:
        ERR_OUT(BNH_EINVAL, err_out, "constant cannot be used as lvalue.\n");
    case BNH_VAR_SCALAR:
        CHECK_ZERO(benesh_var_ver_to_datum(from_var, from_version, &datum), err,
                   err_out, "failed to convert source to datum.\n");
    }

    return (0);
err_out:
    return (err);
}