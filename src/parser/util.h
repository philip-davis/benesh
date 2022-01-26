#ifndef _XC_UTIL_H
#define _XC_UTIL_H

#include <stdio.h>

struct xc_int_stack {
    int val;
    struct xc_int_stack *next;
};

void xc_ipush(struct xc_int_stack **istack, int val)
{
    struct xc_int_stack *top = malloc(sizeof(*top));

    if(!istack) {
        fprintf(stderr, "ERROR: tried to push onto a null integer stack.\n");
        return;
    }

    top = malloc(sizeof(*top));
    top->val = val;
    top->next = *istack;

    *istack = top;
}

int xc_ipop(struct xc_int_stack **istack)
{
    struct xc_int_stack *top;
    int val;

    if(!istack) {
        fprintf(stderr, "ERROR: tried to pop from a null integer stack.\n");
        return (-1);
    } else if(!*istack) {
        fprintf(stderr, "WARNING: tried to pop from an empty integer stack.\n");
        return (-1);
    }

    top = *istack;
    val = top->val;
    *istack = top->next;

    free(top);
    return (val);
}

int xc_itop(struct xc_int_stack *istack)
{
    if(!istack) {
        fprintf(stderr, "WARNING: tried to peek at an empty integer stack.\n");
        return (-1);
    }

    return (istack->val);
}

#endif
