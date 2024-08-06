#define BNH_FROM_LOGGING
#include "benesh-logging.h"

#include <stdio.h>
#include <stdlib.h>

static int f_bnh_debug = 0;
static int f_bnh_trace = 0;
static int bnh_log_rank = -1;

int benesh_debug_enabled() { return (f_bnh_debug); }

int benesh_trace_enabled() { return (f_bnh_trace); }

int benesh_logging_rank() { return (bnh_log_rank); }

int benesh_init_logging(int rank)
{
    const char *envdebug = getenv("BNH_DEBUG");
    const char *envtrace = getenv("BNH_TRACE");
    int err;

    bnh_log_rank = rank;

    if(envdebug) {
        f_bnh_debug = 1;
    } else {
        f_bnh_debug = 0;
    }

    if(envtrace) {
        f_bnh_trace = 1;
    } else {
        f_bnh_trace = 0;
    }

    DEBUG_OUT("initialized logging.\n");

    return (0);
err_out:
    return (err);
}