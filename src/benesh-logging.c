#define BNH_FROM_LOGGING
#include "benesh-logging.h"

#include<stdlib.h>
#include<stdio.h>

static int f_bnh_debug = 0;
static int bnh_log_rank = -1;

int benesh_debug_enabled()
{
    return(f_bnh_debug);
}

int benesh_debug_rank()
{
    return(f_bnh_debug);
}

int benesh_init_logging(int rank)
{
    const char *envdebug = getenv("BNH_DEBUG");
    int err;
    
    bnh_log_rank = rank;

    if(envdebug) {
        f_bnh_debug = 1;
    } else {
        f_bnh_debug = 0;
    }
    DEBUG_OUT("initialized logging.\n");

    return(0);
err_out:
    return(err);
}