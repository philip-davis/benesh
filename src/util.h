#ifndef _BENESH_UTIL
#define _BENESH_UTIL

#include<stdlib.h>
#include<inttypes.h>

struct bnh_pvec {
    void **data;
    size_t len;
    size_t size;
};

typedef void **bnh_pvec_iter;

#define BITER_DEREF(bi, dest) dest = (**(typeof(dest) **)bi);

#define BNH_ITER_END NULL

struct bnh_pvec *benesh_pvec_new(size_t size);

bnh_pvec_iter benesh_pvec_begin(struct bnh_pvec *bvec);

bnh_pvec_iter benesh_pvec_next(struct bnh_pvec *bvec, bnh_pvec_iter biter);

void benesh_pvec_append(struct bnh_pvec *bvec, void *ptr);

#endif // _BENESH_UTIL