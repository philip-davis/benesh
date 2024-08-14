#ifndef _BENESH_OBJ_H
#define _BENESH_OBJ_H

struct benesh_obj;

struct benesh_obj_db;

char *benesh_obj_to_str(struct benesh_obj *obj);

int benesh_obj_nvars(struct benesh_obj *obj, size_t *nvar);

struct benesh_obj *benesh_obj_resolve(struct benesh_obj *obj, int64_t *var_map);

int benesh_obj_free(struct benesh_obj *obj);

int bensh_obj_num_parts(struct benesh_obj *obj);

int benesh_unify_obj_target(struct benesh_obj *obj, struct benesh_obj *target,
                            int64_t **var_map, int *is_viable);

#endif // _BENESH_OBJ_H