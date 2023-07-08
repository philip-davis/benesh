#ifndef __BNH_CPL_WRAPPER_H_
#define __BNH_CPL_WRAPPER_H_

#include <mpi.h>
#include "omegah_wrapper.h"
#include "redev_wrapper.h"
#include "wrapper_common.h"

struct cpl_hndl;

struct field_adapter;

struct cpl_gid_field;

struct cpl_hndl *create_cpl_hndl(const char *wfname, struct omegah_mesh *meshp, struct rdv_ptn *ptnp, int server, MPI_Comm comm);

void close_cpl(struct cpl_hndl *cpl_h);

void mark_cpl_overlap(struct cpl_hndl *cph, struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class);

struct cpl_gid_field *create_gid_field(const char *app_name, const char *field_name, struct cpl_hndl *cphp, struct omegah_mesh *meshp, void *field_buf);

void *cpl_get_field_ptr(struct cpl_gid_field *field);

void cpl_send_field(struct cpl_gid_field *field);

void cpl_recv_field(struct cpl_gid_field *field, double **buffer, size_t *num_elem);

void cpl_combine_fields(struct cpl_hndl *cphp, int num_fields, const char **field_names);

void report_send_recv_timing(struct cpl_gid_field *field, const char *name);


#endif /* __BNH_CPL_WRAPPER_H_ */
