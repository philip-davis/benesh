#ifndef _BENESH_LOGGING_H
#define _BENESH_LOGGING_H

#include <abt.h>
#include <inttypes.h>

#define TRACE_OUT                                                              \
    do {                                                                       \
        if(benesh_trace_enabled()) {                                           \
            ABT_unit_id tid = 0;                                               \
            if(ABT_initialized())                                              \
                ABT_thread_self_id(&tid);                                      \
            fprintf(stderr,                                                    \
                    "App: %s Rank: %i TID: %" PRIu64 " %s:%i (%s): trace\n",   \
                    benesh_logging_name(), benesh_logging_rank(), tid,         \
                    __FILE__, __LINE__, __func__);                             \
        }                                                                      \
    } while(0);

#define DEBUG_OUT(dstr, ...)                                                   \
    do {                                                                       \
        if(benesh_debug_enabled()) {                                           \
            ABT_unit_id tid = 0;                                               \
            if(ABT_initialized())                                              \
                ABT_thread_self_id(&tid);                                      \
            fprintf(stderr,                                                    \
                    "App: %s Rank: %i TID: %" PRIu64 " %s:%i (%s): " dstr,     \
                    benesh_logging_name(), benesh_logging_rank(), tid,         \
                    __FILE__, __LINE__, __func__, ##__VA_ARGS__);              \
        }                                                                      \
    } while(0);

#define WARN_OUT(dstr, ...)                                                    \
    do {                                                                       \
        ABT_unit_id tid = 0;                                                   \
        if(ABT_initialized())                                                  \
            ABT_thread_self_id(&tid);                                          \
        fprintf(stderr,                                                        \
                "WARN: App: %s Rank: %i TID: %" PRIu64 " %s:%i (%s): " dstr,   \
                benesh_logging_name(), benesh_logging_rank(), tid, __FILE__,   \
                __LINE__, __func__, ##__VA_ARGS__);                            \
    } while(0);

#define ERR_OUT(ret, jmp, estr, ...)                                           \
    do {                                                                       \
        ABT_unit_id tid;                                                       \
        ABT_thread_self_id(&tid);                                              \
        fprintf(stderr,                                                        \
                "ERROR: App: %s Rank: %i TID: %" PRIu64 " %s:%i (%s): " estr,  \
                benesh_logging_name(), benesh_logging_rank(), tid, __FILE__,   \
                __LINE__, __func__, ##__VA_ARGS__);                            \
        err = ret;                                                             \
        goto jmp;                                                              \
    } while(0);

#define ERR_OUT_ROOT(ret, jmp, estr, ...)                                      \
    do {                                                                       \
        ABT_unit_id tid;                                                       \
        ABT_thread_self_id(&tid);                                              \
        if(!benesh_logging_rank()) {                                           \
            fprintf(stderr,                                                    \
                    "ERROR: App: %s Rank: %i TID: %" PRIu64 " %s:"             \
                    "%i (%s): " estr,                                          \
                    benesh_logging_name(), benesh_logging_rank(), tid,         \
                    __FILE__, __LINE__, __func__, ##__VA_ARGS__);              \
        }                                                                      \
        err = ret;                                                             \
        goto jmp;                                                              \
    } while(0);

#define CHECK_ZERO(x, ret, jmp, estr, ...)                                     \
    do {                                                                       \
        err = (x);                                                             \
        if(err) {                                                              \
            ERR_OUT(ret, jmp, estr, ##__VA_ARGS__);                            \
        }                                                                      \
    } while(0);

#define CHECK_ZERO_ROOT(x, ret, jmp, estr, ...)                                \
    do {                                                                       \
        err = (x);                                                             \
        if(err) {                                                              \
            ERR_OUT_ROOT(ret, jmp, estr, ##__VA_ARGS__);                       \
        }                                                                      \
    } while(0);

#define CHECK(x, y, ret, jmp, estr, ...)                                       \
    do {                                                                       \
        err = (x);                                                             \
        if(err != y) {                                                         \
            ERR_OUT(ret, jmp, estr, ##__VA_ARGS__);                            \
        }                                                                      \
    } while(0);

#define ASSIGN_NOT_NULL(x, y, ret, jmp, estr, ...)                             \
    do {                                                                       \
        y = (x);                                                               \
        if(!y) {                                                               \
            ERR_OUT(ret, jmp, estr, ##__VA_ARGS__);                            \
        }                                                                      \
    } while(0);

int benesh_trace_enabled();
int benesh_debug_enabled();
int benesh_logging_rank();
int benesh_init_logging(int rank);
char *benesh_logging_name();
int benesh_set_logging_name(const char *name);

#endif // _BENESH_LOGGING_H