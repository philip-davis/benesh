#ifndef _BENESH_H
#define _BENESH_H

#include <mpi.h>
#include <stdint.h>

typedef struct benesh_handle *benesh_handle;
typedef void *benesh_arg;

typedef int (*benesh_method)(benesh_handle, benesh_arg);

int benesh_init(const char *name, const char *conf, MPI_Comm gcomm,
                struct benesh_handle **bnh);

int benesh_bind_method(struct benesh_handle *bnh, const char *name,
                       benesh_method method, void *user_arg);

int benesh_bind_domain(struct benesh_handle *bnh, const char *dom_name,
                       double *grid_offset, double *grid_dims,
                       uint64_t *grid_points);

void benesh_tpoint(struct benesh_handle *bnh, const char *tpname);

int benesh_fini(struct benesh_handle *bnh);

int benesh_get_var_domain(struct benesh_handle *bnh, const char *var_name,
                          char **dom_name, int *ndim, double **lb, double **ub);

void *benesh_get_var_buf(struct benesh_handle *bnh, const char *var_name);

double benesh_get_var_val(struct benesh_handle *bnh, const char *var_name);

#endif /* _BENESH_H */
