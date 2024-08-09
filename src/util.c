#include "util.h"

#include <string.h>

struct bnh_pvec {
    struct bnh_pvec_entry *data;
    size_t len;
    size_t size;
};

struct bnh_pvec *bnh_pvec_new(size_t size)
{
    struct bnh_pvec *bvec = calloc(sizeof(*bvec), 1);

    bvec->data = calloc(sizeof(*bvec->data), size);
    bvec->size = size;

    return (bvec);
}

bnh_pvec_iter bnh_pvec_begin(struct bnh_pvec *bvec)
{
    return (bvec->len ? &bvec->data[0] : BNH_ITER_END);
}

bnh_pvec_iter bnh_pvec_next(struct bnh_pvec *bvec, bnh_pvec_iter biter)
{
    uint64_t idx;

    idx = ((uint64_t)biter - (uint64_t)bvec->data) / sizeof(*biter);
    return ((++idx < bvec->len) ? &bvec->data[idx] : BNH_ITER_END);
}

void bnh_pvec_append(struct bnh_pvec *bvec, void *ptr, int id)
{
    if(bvec->len >= bvec->size) {
        bvec->size *= 2;
        bvec->data = realloc(bvec->data, sizeof(*bvec->data) * bvec->size);
    }
    bvec->data[bvec->len].ptr = ptr;
    bvec->data[bvec->len++].id = id;
}

void bnh_pvec_destroy(struct bnh_pvec *bvec, int free_contents)
{
    void *ptr;
    bnh_pvec_iter bi;

    if(bvec) {
        if(free_contents) {
            BNH_PVEC_FOREACH(ptr, bi, bvec) { free(ptr); }
        }
        free(bvec->data);
        free(bvec);
    }
}

void *bnh_pvec_get_by_id(struct bnh_pvec *bvec, int id)
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

int bnh_pvec_get_len(struct bnh_pvec *bvec)
{
    if(!bvec) {
        return (-1);
    }
    return (bvec->len);
}

char *bnh_bracket_str(const char *prefix, const char *middle,
                      const char *postfix)
{
    char *str;
    int str_len = 1;

    if(prefix) {
        str_len += strlen(prefix);
    }
    if(middle) {
        str_len += strlen(middle);
    }
    if(postfix) {
        str_len += strlen(postfix);
    }

    if(!str_len) {
        return (strdup(""));
    }

    str = malloc(str_len);
    if(!str) {
        return (NULL);
    }
    *str = '\0';

    // if I use sprintf, NULL arguments are printed as (null)
    if(prefix) {
        strcat(str, prefix);
    }
    if(middle) {
        strcat(str, middle);
    }
    if(postfix) {
        strcat(str, postfix);
    }

    return (str);
}