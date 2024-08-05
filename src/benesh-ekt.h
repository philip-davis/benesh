#ifndef _BENESH_EKT_H
#define _BENESH_EKT_H

#define BNH_EKT_TP 0
#define BNH_EKT_WORK 1
#define BNH_EKT_FINI 2

int benesh_init_ekt(struct benesh_handle *bnh);

int benesh_ekt_xconnect(struct benesh_handle *bnh, int wait);

#endif // _BENESH_EKT_H