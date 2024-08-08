#include "util.h"

struct bnh_pvec {
    struct bnh_pvec_entry *data;
    size_t len;
    size_t size;
};

struct bnh_pvec *benesh_pvec_new(size_t size)
{
    struct bnh_pvec *bvec = calloc(sizeof(*bvec), 1);

    bvec->data = calloc(sizeof(*bvec->data), size);
    bvec->size = size;

    return (bvec);
}

bnh_pvec_iter benesh_pvec_begin(struct bnh_pvec *bvec)
{
    return (bvec->len ? &bvec->data[0] : BNH_ITER_END);
}

bnh_pvec_iter benesh_pvec_next(struct bnh_pvec *bvec, bnh_pvec_iter biter)
{
    uint64_t idx;

    idx = ((uint64_t)biter - (uint64_t)bvec->data) / sizeof(*biter);
    return ((++idx < bvec->len) ? &bvec->data[idx] : BNH_ITER_END);
}

void benesh_pvec_append(struct bnh_pvec *bvec, void *ptr, int id)
{
    if(bvec->len >= bvec->size) {
        bvec->size *= 2;
        bvec->data = realloc(bvec->data, sizeof(*bvec->data) * bvec->size);
    }
    bvec->data[bvec->len].ptr = ptr;
    bvec->data[bvec->len++].id = id;
}

void *benesh_pvec_get_by_id(struct bnh_pvec *bvec, int id)
{
    bnh_pvec_iter bi;
    void *ptr;

    BNH_PVEC_FOREACH(ptr, bi, bvec)
    {
        if(bi->id == id) {
            return (ptr);
        }
    }

    return (NULL);
}

int benesh_pvec_get_len(struct bnh_pvec *bvec)
{
    if(!bvec) {
        return (-1);
    }
    return (bvec->len);
}