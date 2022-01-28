#ifndef _BENESH_IHASH_H
#define _BENESH_IHASH_H

#include<stdlib.h>

struct xc_int_hash_node {
    struct xc_int_hash_node *next;
    int idx;
    void *val;
};

struct xc_int_hash_map {
    struct xc_int_hash_node **hash;
    int hash_size;
    int entries;
    int seed;
};

struct xc_int_hash_map *xc_new_ihash_map(int size, int seed)
{
    struct xc_int_hash_map *ihash = malloc(sizeof(*ihash));
    
    ihash->hash_size = size;
    ihash->seed = seed;
    ihash->hash = calloc(size, sizeof(*ihash->hash));

    return(ihash);
}

void xc_ihash_map_add(struct xc_int_hash_map *ihash, int idx, void *val)
{
   int hidx = (idx * ihash->seed) % ihash->hash_size;
   struct xc_int_hash_node **hnode;

    if(hidx < 0) {
        hidx += ihash->hash_size;
    }

    hnode = &ihash->hash[hidx];

    while(*hnode) {
        hnode = &(*hnode)->next;
    }
    *hnode = calloc(1, sizeof(**hnode));
    (*hnode)->idx = idx;
    (*hnode)->val = val;
}

void *xc_ihash_map_lookup(struct xc_int_hash_map *ihash, int idx)
{
    int hidx = (idx * ihash->seed) % ihash->hash_size;
    struct xc_int_hash_node *hnode;

    if(hidx < 0) {
        hidx += ihash->hash_size;
    }

    hnode = ihash->hash[hidx];

    while(hnode) {
        if(hnode->idx == idx) {
            return(hnode->val);
        }
        hnode = hnode->next;
    }

    return(NULL);
}

#endif
