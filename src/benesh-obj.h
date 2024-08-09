#ifndef _BENESH_OBJ_H
#define _BENESH_OBJ_H

struct benesh_obj;

char *benesh_obj_to_str(struct benesh_obj *obj);

struct benesh_obj *benesh_obj_resolve(struct benesh_obj *obj, int64_t *var_map);

int benesh_obj_free(struct benesh_obj *obj);

#endif // _BENESH_OBJ_H