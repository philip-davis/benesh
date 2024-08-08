#ifndef _BENESH_H
#define _BENESH_H

#include <errno.h>
#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define BNH_EINVAL EINVAL /* invalid argument */
#define BNH_EPERM                                                              \
    EPERM                 /* bad external state (e.g. MPI not initialized yet) \
                           */
#define BNH_EIO EIO       /* communication failure */
#define BNH_EFAULT EFAULT /* bad pointer */
#define BNH_ESRCH ESRCH   /* no such component */
#define BNH_ENOMEM ENOMEM /* memory allocation failure */

#define BNH_EINCONST -1 /* inconsistent state between ranks */
#define BNH_EEKT -2     /* EKT failure */
#define BNH_ECONF -3    /* configuration error */
#define BNH_ESTATE -4   /* inconsistent internal state */
#define BNH_EABT -5     /* argobots failure */
#define BNH_ESYNC -6    /* inconsistent state between components */

typedef struct benesh_handle *benesh_app_id;
typedef void *benesh_arg;

typedef int (*benesh_method)(benesh_app_id, benesh_arg);

int benesh_init(const char *name, const char *conf, MPI_Comm gcomm, int dummy,
                int wait, struct benesh_handle **bnh);

int benesh_bind_method(struct benesh_handle *bnh, const char *name,
                       benesh_method method, void *user_arg);

int benesh_bind_var(struct benesh_handle *bnh, const char *var_name, void *buf);

void *benesh_bind_var_mesh(struct benesh_handle *bnh, const char *var_name,
                           int *idx, unsigned int idx_len);

void *benesh_bind_field_mpient(struct benesh_handle *bnh, const char *var_name,
                               int idx, const char *rcn_file, MPI_Comm comm,
                               void *buffer, int length, int participates);

void *benesh_bind_field_dummy(struct benesh_handle *bnh, const char *var_name,
                              int idx, int participates);

int benesh_bind_grid_domain(struct benesh_handle *bnh, const char *dom_name,
                            double *grid_offset, double *grid_dims,
                            uint64_t *grid_points, int alloc);

int benesh_bind_mesh_domain(struct benesh_handle *bnh, const char *dom_name,
                            const char *grid_file, const char *cpn_file,
                            int alloc);

int benesh_bind_field_domain(struct benesh_handle *bnh, const char *dom_name);

void benesh_tpoint(struct benesh_handle *bnh, const char *tpname);

void benesh_touchpoint(benesh_app_id bnh, const char *tpname);

int benesh_fini(struct benesh_handle *bnh);

int benesh_get_var_domain(struct benesh_handle *bnh, const char *var_name,
                          char **dom_name, int *ndim, double **lb, double **ub);

void *benesh_get_var_buf(struct benesh_handle *bnh, const char *var_name,
                         uint64_t *size);

typedef enum bnh_order { BNH_ROW_MAJOR, BNH_COL_MAJOR } bnh_order_t;

typedef enum bnh_storage_type { BNH_GRID, BNH_MESH, BNH_SET } bnh_storage_type;

typedef struct bnh_storage_def {
    bnh_storage_type type;
    union {
        struct {
            enum bnh_order order;
            int ndim;
            int *dims;
            int *ghosts;
        };
    };
} *bnh_storage_def_t;

bnh_storage_def_t benesh_grid_def(const int ndim, int *dims, bnh_order_t order);

void benesh_grid_ghosts(bnh_storage_def_t gdef, int *depths);

void benesh_grid_ghosts_uniform(bnh_storage_def_t gdef, int depth);

typedef enum benesh_datatype {
    BNH_INT,
    BNH_LONG,
    BNH_FLOAT,
    BNH_DOUBLE,
    BNH_BOOL,
    BNH_BYTES
} benesh_datatype;

enum benesh_fragment_type { BNH_TILE, BNH_DISC_TILE, BNH_GEO_TILE, BNH_SPANS };

struct benesh_tile_fragment {
    int ndim;
    double *lb;
    double *ub;
};

struct benesh_discrete_fragment {
    int ndim;
    uint64_t *lb;
    uint64_t *ub;
};

struct benesh_geotile_fragment {
    double lb[2];
    double ub[2];
};

struct benensh_spans_fragment {
    int count;
    uint64_t *starts;
    uint64_t *spans;
};

typedef struct benesh_domain_fragment {
    enum benesh_fragment_type type;
    union {
        struct benesh_tile_fragment tile;
        struct benesh_discrete_fragment dtile;
        struct benesh_geotile_fragment gtile;
        struct benensh_spans_fragment spans;
    };
} *benesh_domain_fragment_t;

benesh_domain_fragment_t benesh_domain_geotile_decompose(benesh_app_id bnh,
                                                         const char *domain,
                                                         double lb[2],
                                                         double ub[2]);

int benesh_bind_var_with_size(benesh_app_id bnh, const char *var_name,
                              bnh_storage_def_t sdef,
                              benesh_domain_fragment_t dfrag, size_t dsize);

int benesh_bind_var_with_type(benesh_app_id bnh, const char *var_name,
                              bnh_storage_def_t sdef,
                              benesh_domain_fragment_t dfrag,
                              benesh_datatype dtype);

int benesh_bind_grid_domain(benesh_app_id bnh, const char *dom_name,
                            double *grid_offset, double *grid_dims,
                            uint64_t *grid_points, int alloc);

int benesh_bind_mesh_domain(benesh_app_id bnh, const char *dom_name,
                            const char *grid_file, const char *cpn_file,
                            int alloc);

// double benesh_get_var_val(struct benesh_handle *bnh, const char *var_name);

typedef enum benesh_type_t {
    BNH_TYPE_INT,
    BNH_TYPE_REAL,
    BNH_TYPE_ERR
} benesh_type_t;

typedef long benesh_int_t;
typedef double benesh_real_t;

/**
 * Retrieve the value of a named workflow value as an integer.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a referece to a benesh_int_t variable, which will be
 * populated with the output value
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not an integer type)
 */
int benesh_get_var_ival(struct benesh_handle *bnh, const char *var_name,
                        benesh_int_t *val);

/**
 * Retrieve the value of a named workflow value as a real value.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a referece to a benesh_real_t variable, which will be
 * populated with the output value
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not a real type)
 */
int benesh_get_var_rval(struct benesh_handle *bnh, const char *var_name,
                        benesh_real_t *val);

/**
 * Retrieve the value of a named workflow value without specifying the type.
 * Allocates memory to hold the value.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a reference to a buffer pointer. The buffer pointer will be
 * populated with the address of the allocated memory containing the output
 * value.
 * @return the type of var_name
 */
benesh_type_t benesh_get_var_val(struct benesh_handle *bnh,
                                 const char *var_name, void **val);

/**
 * Retrieve the contents of a named workflow array. Allocates memory to hold the
 * results.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a reference to a buffer pointer. The buffer pointer will be
 * populated with the address of an allocated range of memory containing a copy
 * of the contents of the workflow variable.
 * @param[out] type the data type of the array
 * @return the number of elements in the array. Negative indicates failure (e.g.
 * if var_name is not an array)
 */
int benesh_get_var_array(struct benesh_handle *bnh, const char *var_name,
                         void **val, benesh_type_t *type);

/** Set the value of a named workflow value as an integer. The new value will be
 * propagated across the workflow, with no guarantees about consistency between
 * ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] val the new value for var_name
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not an integer type)
 */
int benesh_update_var_ival(struct benesh_handle *bnh, const char *var_name,
                           benesh_int_t val);

/** Set the value of a named workflow value as a real number. The new value will
 * be propagated across the workflow, with no guarantees about consistency
 * between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] val the new value for var_name
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not a real type)
 */
int benesh_update_var_rval(struct benesh_handle *bnh, const char *var_name,
                           benesh_real_t val);

/** Update the contents of a named workflow array. The entire array will be
 * overwritten in one call. The new values will be propagated across the
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] vals a pointer to an array, the contents of which will be copied
 * into the workflow array on all workflow processes.
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not an array)
 */
int benesh_update_var_array(struct benesh_handle *bnh, const char *var_name,
                            void *vals);

/** Update the contents of a single element of a workflow array of integers. The
 * new values will be propagated across the workflow, with no guarantees about
 * consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] idx array index to update
 * @param[in] val the new value for var_name[idx]
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not an array of integers)
 */
int benesh_update_var_array_ival(struct benesh_handle *bnh,
                                 const char *var_name, int idx,
                                 benesh_int_t val);

/** Update the contents of a single element of a workflow array of reals. The
 * new values will be propagated across the workflow, with no guarantees about
 * consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] idx array index to update
 * @param[in] val the new value for var_name[idx]
 * @return zero indicates success, negative indicates failure (e.g. if var_name
 * is not an array of reals)
 */
int benesh_update_var_array_rval(struct benesh_handle *bnh,
                                 const char *var_name, int idx,
                                 benesh_real_t val);

/** Retrieve the number of elements in a workflow array.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @return the size of the array. Negative indicates failure (e.g. if var_name
 * is not an array)
 */
int benesh_get_array_len(struct benesh_handle *bnh, const char *var_name);

/** Resize an workflow array of integers
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable ot retrieve
 * @param[in] size the new size of the array
 * @param[in] fill if the array is being enlarged, the fill value to use
 * @return if successful, the previous size of the array. Negative on failure
 * (e.g. var_name is not an array of integers)
 */
int benesh_resize_iarray(struct benesh_handle *bnh, const char *var_name,
                         size_t size, benesh_int_t fill);

/** Resize an workflow array of reals
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable ot retrieve
 * @param[in] size the new size of the array
 * @param[in] fill if the array is being enlarged, the fill value to use
 * @return if successful, the previous size of the array. Negative on failure
 * (e.g. var_name is not an array of reals)
 */
int benesh_resize_rarray(struct benesh_handle *bnh, const char *var_name,
                         size_t size, benesh_real_t fill);

void benesh_unify_mesh_data(struct benesh_handle *bnh, const char *var_name);

#if defined(__cplusplus)
}
#endif

#endif /* _BENESH_H */
