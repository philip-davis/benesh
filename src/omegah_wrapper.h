#ifndef _BNH_OH_WRAPPER_
#define _BNH_OH_WRAPPER_

struct omegah_mesh;

struct omegah_mesh *new_oh_mesh(const char *meshFile);

struct rdv_ptn *create_oh_partition(struct omegah_mesh *meshp, const char *cpnFile);

#endif
