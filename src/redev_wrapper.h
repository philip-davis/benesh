#ifndef _BNH_RDV_WRAPPER_
#define _BNH_RDV_WRAPPER_

#include <stdint.h>
#include <stdlib.h>

struct rdv_comm;
struct rdv_ptn;

struct rdv_comm *new_rdv_comm(void *mpiComm_v, const int rdvRanks, int isRdv);

struct rdv_comm *new_rdv_comm_ptn(MPI_Comm *comm, const char *name, int isRdv,
                                  struct rdv_ptn *rptn);

void rdv_layout(struct rdv_comm *comm, int count, uint32_t *dest,
                uint32_t *offset);

void rdv_send(struct rdv_comm *comm, int rank, void *buffer);

void rdv_recv(struct rdv_comm *comm, int rank, void **buffer, size_t *buflen);
#endif
