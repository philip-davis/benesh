#ifndef _BENESH_DATA_H
#define _BENESH_DATA_H

#include "benesh-obj.h"

struct benesh_var;

int benesh_var_assign(struct benesh_var *from_var,
                      struct benesh_obj *from_version,
                      struct benesh_var *to_var, struct benesh_obj *to_version);

#endif // _BENESH_VARS_H