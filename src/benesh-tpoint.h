#ifndef _BENESH_TPOINT_H
#define _BENESH_TPOINT_H

#include "benesh-obj.h"
#include "benesh-tpoint.h"
#include "benesh.h"
#include "util.h"

struct benesh_touchpoint;

typedef struct bnh_pvec *benesh_tpoints;

struct benesh_touchpoint *benesh_tp_get_by_id(benesh_tpoints tpoints,
                                              int tp_id);

int benesh_tp_get_nvar(struct benesh_touchpoint *tpoint, size_t *nvar);

int benesh_tp_get_id(struct benesh_touchpoint *tpoint, int *tp_id);

char *benesh_tp_to_str(struct benesh_touchpoint *tpoint, int64_t *var_map);

struct benesh_component *benesh_tp_get_comp(struct benesh_touchpoint *tpoint);

int benesh_tp_handle(struct benesh_handle *bnh,
                     struct benesh_touchpoint *tpoint, int64_t *var_map);

int benesh_tp_find_viable(struct bnh_pvec *tpoint_rules,
                          struct benesh_component *comp, struct benesh_obj *obj,
                          struct benesh_touchpoint **tpoint, int64_t **var_map);

int benesh_tp_is_complete(struct benesh_handle *bnh,
                          struct benesh_touchpoint *tpoint, int64_t *var_map,
                          int *eout);

#endif // _BENESH_TPOINT_H