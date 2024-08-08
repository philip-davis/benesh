#ifndef _BENESH_UTIL
#define _BENESH_UTIL

#include <inttypes.h>
#include <stdlib.h>

struct bnh_pvec_entry {
    void *ptr;
    int id;
};

struct bnh_pvec;

typedef struct bnh_pvec_entry *bnh_pvec_iter;

#define BITER_DEREF(bi, dest) dest = (**(typeof(dest) **)bi);

#define BNH_ITER_END NULL

#define BNH_PVEC_FOREACH(x, bi, vec)                                           \
    for(bi = benesh_pvec_begin(vec); (bi != BNH_ITER_END) && (x = bi->ptr);    \
        bi = benesh_pvec_next(vec, bi))

struct bnh_pvec *benesh_pvec_new(size_t size);

bnh_pvec_iter benesh_pvec_begin(struct bnh_pvec *bvec);

bnh_pvec_iter benesh_pvec_next(struct bnh_pvec *bvec, bnh_pvec_iter biter);

void benesh_pvec_append(struct bnh_pvec *bvec, void *ptr, int id);

void *benesh_pvec_get_by_id(struct bnh_pvec *bvec, int id);

int benesh_pvec_get_len(struct bnh_pvec *bvec);

#endif // _BENESH_UTIL