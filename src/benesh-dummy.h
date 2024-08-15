#ifndef _BENESH_DUMMY_H
#define _BENESH_DUMMY_H

enum bnh_dummy_cmd_type { BNH_DCMD_DATA, BNH_DCMD_TERM };

typedef int (*dcmd_cb)(int id, void *arg, void *payload);

struct benesh_dummy_cmd_handler {
    int type;
    int id;
    void *saved_arg;
    void *payload;
};

#endif // _BENESH_DUMMY_H