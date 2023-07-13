#ifndef _BENESH_H
#define _BENESH_H

#include <mpi.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct benesh_handle *benesh_app_id;
typedef void *benesh_arg;

typedef int (*benesh_method)(benesh_app_id, benesh_arg);

int benesh_init(const char *name, const char *conf, MPI_Comm gcomm, int wait,
                struct benesh_handle **bnh);

int benesh_bind_method(struct benesh_handle *bnh, const char *name,
                       benesh_method method, void *user_arg);

int benesh_bind_var(struct benesh_handle *bnh, const char *var_name, void *buf);

void *benesh_bind_var_mesh(struct benesh_handle *bnh, const char *var_name, int *idx, unsigned int idx_len);

void *benesh_bind_field_mpient(struct benesh_handle *bnh, const char *var_name, int idx, const char *rcn_file, MPI_Comm comm, void *buffer, int length, int participates);

void *benesh_bind_field_dummy(struct benesh_handle *bnh, const char *var_name, int idx, int participates);

int benesh_bind_grid_domain(struct benesh_handle *bnh, const char *dom_name,
                            double *grid_offset, double *grid_dims,
                            uint64_t *grid_points, int alloc);

int benesh_bind_mesh_domain(struct benesh_handle *bnh, const char *dom_name,
                            const char *grid_file, const char *cpn_file, int alloc);

void benesh_tpoint(struct benesh_handle *bnh, const char *tpname);

int benesh_fini(struct benesh_handle *bnh);

int benesh_get_var_domain(struct benesh_handle *bnh, const char *var_name,
                          char **dom_name, int *ndim, double **lb, double **ub);

void *benesh_get_var_buf(struct benesh_handle *bnh, const char *var_name, uint64_t *size);

double benesh_get_var_val(struct benesh_handle *bnh, const char *var_name);

void benesh_unify_mesh_data(struct benesh_handle *bnh, const char *var_name);

#if defined(__cplusplus)
}
#endif

#endif /* _BENESH_H */
