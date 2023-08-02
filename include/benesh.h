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

int benesh_init(const char *name, const char *conf, MPI_Comm gcomm, int dummy, int wait,
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

int benesh_bind_field_domain(struct benesh_handle *bnh, const char *dom_name);

void benesh_tpoint(struct benesh_handle *bnh, const char *tpname);

int benesh_fini(struct benesh_handle *bnh);

int benesh_get_var_domain(struct benesh_handle *bnh, const char *var_name,
                          char **dom_name, int *ndim, double **lb, double **ub);

void *benesh_get_var_buf(struct benesh_handle *bnh, const char *var_name, uint64_t *size);

//double benesh_get_var_val(struct benesh_handle *bnh, const char *var_name);

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
 * @param[out] val a referece to a benesh_int_t variable, which will be populated with the output value
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not an integer type)
 */
int benesh_get_var_ival(struct benesh_handle *bnh, const char *var_name, benesh_int_t *val);

/**
 * Retrieve the value of a named workflow value as a real value.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a referece to a benesh_real_t variable, which will be populated with the output value
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not a real type)
 */
int benesh_get_var_rval(struct benesh_handle *bnh, const char *var_name, benesh_real_t *val);

/**
 * Retrieve the value of a named workflow value without specifying the type. Allocates memory to hold the value.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a reference to a buffer pointer. The buffer pointer will be populated with the address
 *  of the allocated memory containing the output value.
 * @return the type of var_name
 */
benesh_type_t benesh_get_var_val(struct benesh_handle *bnh, const char *var_name, void **val);

/**
 * Retrieve the contents of a named workflow array. Allocates memory to hold the results.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[out] val a reference to a buffer pointer. The buffer pointer will be populated with the address of an
 *  allocated range of memory containing a copy of the contents of the workflow variable.
 * @param[out] type the data type of the array
 * @return the number of elements in the array. Negative indicates failure (e.g. if var_name is not an array)
 */
int benesh_get_var_array(struct benesh_handle *bnh, const char *var_name, void **val, benesh_type_t *type);

/** Set the value of a named workflow value as an integer. The new value will be propagated across the
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] val the new value for var_name
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not an integer type)
 */
int benesh_update_var_ival(struct benesh_handle *bnh, const char *var_name, benesh_int_t val);

/** Set the value of a named workflow value as a real number. The new value will be propagated across the 
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] val the new value for var_name
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not a real type)
 */
int benesh_update_var_rval(struct benesh_handle *bnh, const char *var_name, benesh_real_t val);

/** Update the contents of a named workflow array. The entire array will be overwritten in one call. The new values will be propagated across the
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] vals a pointer to an array, the contents of which will be copied into the workflow array on all workflow processes.
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not an array)
 */
int benesh_update_var_array(struct benesh_handle *bnh, const char *var_name, void *vals);

/** Update the contents of a single element of a workflow array of integers. The new values will be propagated across the
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] idx array index to update
 * @param[in] val the new value for var_name[idx]
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not an array of integers)
 */
int benesh_update_var_array_ival(struct benesh_handle *bnh, const char *var_name, int idx, benesh_int_t val);

/** Update the contents of a single element of a workflow array of reals. The new values will be propagated across the
 * workflow, with no guarantees about consistency between ranks or components.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @param[in] idx array index to update
 * @param[in] val the new value for var_name[idx]
 * @return zero indicates success, negative indicates failure (e.g. if var_name is not an array of reals)
 */
int benesh_update_var_array_rval(struct benesh_handle *bnh, const char *var_name, int idx, benesh_real_t val);

/** Retrieve the number of elements in a workflow array.
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable to retrieve
 * @return the size of the array. Negative indicates failure (e.g. if var_name is not an array)
 */
int benesh_get_array_len(struct benesh_handle *bnh, const char *var_name);

/** Resize an workflow array of integers
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable ot retrieve
 * @param[in] size the new size of the array
 * @param[in] fill if the array is being enlarged, the fill value to use
 * @return if successful, the previous size of the array. Negative on failure (e.g. var_name is not an array of integers)
 */
int benesh_resize_iarray(struct benesh_handle *bnh, const char *var_name, size_t size, benesh_int_t fill);

/** Resize an workflow array of reals
 * @param[in] bnh the handle into the Benesh library
 * @param[in] var_name the name of the variable ot retrieve
 * @param[in] size the new size of the array
 * @param[in] fill if the array is being enlarged, the fill value to use
 * @return if successful, the previous size of the array. Negative on failure (e.g. var_name is not an array of reals)
 */
int benesh_resize_rarray(struct benesh_handle *bnh, const char *var_name, size_t size, benesh_real_t fill);

void benesh_unify_mesh_data(struct benesh_handle *bnh, const char *var_name);

#if defined(__cplusplus)
}
#endif

#endif /* _BENESH_H */
