#ifndef __BNH_CPL_WRAPPER_H_
#define __BNH_CPL_WRAPPER_H_

#include <mpi.h>
#include "omegah_wrapper.h"
#include "redev_wrapper.h"
#include "wrapper_common.h"

struct cpl_hndl;

struct field_adapter;

struct cpl_gid_field;

struct rcn_handle;

struct cpl_hndl *create_cpl_hndl(const char *wfname, struct omegah_mesh *meshp, struct rdv_ptn *ptnp, int server, MPI_Comm comm);

void close_cpl(struct cpl_hndl *cpl_h);

struct app_hndl *add_application(struct cpl_hndl *cpl_h, const char *name, const char *path);

void app_begin_recv_phase(struct app_hndl *app_h);
void app_end_recv_phase(struct app_hndl *app_h);
void app_begin_send_phase(struct app_hndl *app_h);
void app_end_send_phase(struct app_hndl *app_h);

struct field_adapter *create_dummy_adapter();
struct field_adapter *create_mpient_adapter(struct app_hndl *app_h, const char *name, struct rcn_handle *rcn_h, MPI_Comm comm, void *data, int size, enum bnh_data_type data_type, int min_class, int max_class);

struct rcn_handle *get_rcn_from_file(const char *fname, MPI_Comm comm);

struct field_adapter *create_omegah_adapter(struct app_hndl *app_h, const char *name, struct omegah_mesh *meshp, enum bnh_data_type data_type);

void mark_cpl_overlap(struct cpl_hndl *cph, struct omegah_mesh *meshp, struct rdv_ptn *rptn, int min_class, int max_class);

struct field_handle *cpl_add_field(struct cpl_hndl *cpl_h, const char *app_name, const char *name, int participates);

struct cpl_gid_field *create_gid_field(const char *app_name, const char *field_name, struct cpl_hndl *cphp, struct omegah_mesh *meshp, void *field_buf);

void *cpl_get_field_ptr(struct field_handle *field);

void cpl_send_field(struct field_handle *field);

void cpl_recv_field(struct field_handle *field);

void cpl_combine_fields(struct cpl_hndl *cphp, int num_fields, const char **field_names);

void report_send_recv_timing(struct cpl_gid_field *field, const char *name);


#endif /* __BNH_CPL_WRAPPER_H_ */
