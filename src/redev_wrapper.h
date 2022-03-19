#ifndef _BNH_RDV_WRAPPER_
#define _BNH_RDV_WRAPPER_

#include <stdint.h>
#include <stdlib.h>

struct rdv_comm;

struct rdv_comm *new_rdv_comm(void *mpiComm_v, const int rdvRanks, int isRdv);

void rdv_send(struct rdv_comm *comm, int count, int32_t *dest, int32_t *offset,
              size_t buflen, int32_t *buffer);

void rdv_recv(struct rdv_comm *comm, int rank, void **buffer);

#endif
