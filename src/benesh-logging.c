#include "benesh-logging.h"
#include "benesh.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static atomic_int f_bnh_debug = 0;
static atomic_int f_bnh_trace = 0;
static atomic_int bnh_log_rank = -1;
static char *bnh_log_name = NULL;

int benesh_debug_enabled() { return (f_bnh_debug); }

int benesh_trace_enabled() { return (f_bnh_trace); }

int benesh_logging_rank() { return (bnh_log_rank); }

char *benesh_logging_name() { return (bnh_log_name); }

int benesh_set_logging_name(const char *name)
{
    TRACE_OUT;
    int err;

    if(!name || !*name) {
        ERR_OUT(BNH_EFAULT, err_out, "name must be present.\n");
    }

    if(bnh_log_name) {
        free(bnh_log_name);
    }

    ASSIGN_NOT_NULL(strdup(name), bnh_log_name, BNH_ENOMEM, err_out,
                    "buffer allocation failure.\n");
    bnh_log_name = strdup(name);

    return (0);
err_out:
    return (err);
}

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

    bnh_log_name = strdup("unset");

    DEBUG_OUT("initialized logging.\n");

    return (0);
err_out:
    return (err);
}