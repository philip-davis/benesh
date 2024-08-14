#ifndef _BENESH_TPOINT_H
#define _BENESH_TPOINT_H

#include "benesh.h"
#include "util.h"

struct benesh_touchpoint;

typedef struct bnh_pvec *benesh_tpoints;

struct benesh_touchpoint *benesh_tp_get_by_id(benesh_tpoints tpoints,
                                              int tp_id);

int benesh_tp_get_nvar(struct benesh_touchpoint *tpoint, size_t *nvar);

char *benesh_tp_to_str(struct benesh_touchpoint *tpoint, int64_t *var_map);

struct benesh_component *benesh_tp_get_comp(struct benesh_touchpoint *tpoint);

int benesh_tp_handle(struct benesh_handle *bnh,
                     struct benesh_touchpoint *tpoint, int64_t *var_map);

#endif // _BENESH_TPOINT_H