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
    for(bi = bnh_pvec_begin(vec); (bi != BNH_ITER_END) && (x = bi->ptr);       \
        bi = bnh_pvec_next(vec, bi))

#define BNH_PVEC_FOREACH_ID(x, eid, bi, vec)                                   \
    for(bi = bnh_pvec_begin(vec);                                              \
        (bi != BNH_ITER_END) && (x = bi->ptr) && (eid = bi->id);               \
        bi = bnh_pvec_next(vec, bi))

struct bnh_pvec *bnh_pvec_new(size_t size);

bnh_pvec_iter bnh_pvec_begin(struct bnh_pvec *bvec);

bnh_pvec_iter bnh_pvec_next(struct bnh_pvec *bvec, bnh_pvec_iter biter);

void bnh_pvec_append(struct bnh_pvec *bvec, void *ptr, int id);

void *bnh_pvec_get(struct bnh_pvec *bvec, int idx);

void *bnh_pvec_get_by_id(struct bnh_pvec *bvec, int id);

int bnh_pvec_get_len(struct bnh_pvec *bvec);

void bnh_pvec_destroy(struct bnh_pvec *bvec, int free_contents);

char *bnh_bracket_str(const char *prefix, const char *middle,
                      const char *postfix);

struct bnh_hash;

struct bnh_hash *bnh_hash_new(size_t size, int seed);

void bnh_hash_add_entry(struct bnh_hash *hash, int id, void *ptr);

void *bnh_hash_lookup(struct bnh_hash *hash, long id);

struct bnh_dict;

struct bnh_dict *bnh_dict_new(size_t size);

void bnh_dict_assign(struct bnh_dict *dict, const char *key, void *val);

int bnh_dict_lookup(struct bnh_dict *dict, const char *key, void **result);

#endif // _BENESH_UTIL