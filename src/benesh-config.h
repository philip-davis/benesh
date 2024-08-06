#ifndef _BENESH_CONFIG_H
#define _BENESH_CONFIG_H

#include "benesh-types.h"
#include "parser/xc_config.h"

// TODO
struct xc_config *benesh_config_load(const char *conf_file, MPI_Comm comm);

#endif // _BENESH_CONFIG_H