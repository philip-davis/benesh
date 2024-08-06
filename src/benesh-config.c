#include "benesh-config.h"
#include "benesh-logging.h"
#include "parser/xc_config.h"

#include <mpi.h>

struct xc_config *benesh_config_load(const char *conf_file, MPI_Comm comm)
{
    TRACE_OUT;
    struct xc_config *conf;
    int err;

    // conf = xc_fparse(conf_file, comm);
    if(!conf) {
        ERR_OUT(BNH_ECONF, err_out, "configuration parsing failed.\n");
    }

    // ...

    return (conf);
err_out:
    return (NULL);
}