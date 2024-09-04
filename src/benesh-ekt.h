#ifndef _BENESH_EKT_H
#define _BENESH_EKT_H

#include <ekt.h>
#include <margo.h>
#include <mpi.h>

#include "benesh-cohort.h"
#include "benesh-tpoint.h"

#define BNH_EKT_TP 0
#define BNH_EKT_WORK 1
#define BNH_EKT_FINI 2

struct bnhekt_handle;

struct bnhekt_handle *benesh_ekt_init(const char *name, MPI_Comm comm,
                                      margo_instance_id mid, void *bnhv);

int benesh_ekt_send_fini(struct bnhekt_handle *bekth, uint32_t comp_id);

int benesh_ekt_xconnect(struct bnhekt_handle *bekth, struct benesh_cohort *bco,
                        MPI_Comm comm, int wait);

int benesh_ekt_announce_tp(struct bnhekt_handle *bekth,
                           struct benesh_touchpoint *tpoint, int64_t *var_map);

int benesh_ekt_fini(struct bnhekt_handle *bekth);

#endif // _BENESH_EKT_H