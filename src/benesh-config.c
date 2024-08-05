#include "benesh-config.h"
#include "xc_config.h"
#include "benesh-logging.h"

int benesh_config(struct benesh_handle *bnh, const char *conf_file)
{
    int err;

    bnh->conf = xc_fparse(conf_file, bnh->gcomm);
    if(!bnh->conf) {
        ERR_OUT(BNH_ECONF, err_out, "configuration parsing failed.\n");
    }

    // ...

    return(0);
err_out:
    return(err);
}