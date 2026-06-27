/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Shared quicksort core, so the algorithm is defined in a single place and
    used by both the ISO C qsort() and the GNU/BSD extension qsort_r().

    Original algorithm from NetBSD: Bentley & McIlroy, "Engineering a Sort
    Function".

    The header is used in two phases by each implementation file:

      1. Included once at file scope to provide the (context-independent)
         swap helpers.

      2. Included again *inside* the body of the sort function, with
         QSORT_EMIT_BODY defined, to emit the algorithm itself.  Before that
         second include the function must define:

           QSORT_CMP(a, b)        - expression comparing the elements pointed
                                    to by char *a and char *b (<0/0/>0).  It
                                    may capture extra context (e.g. an arg
                                    pointer), which is how qsort_r() threads
                                    its argument through to the comparator.
           QSORT_RECURSE(a, n)    - tail recursion on the sort function for the
                                    sub-array starting at char *a with n
                                    elements (the element size es is implicit).

         and the enclosing function must name its parameters a (void *),
         n (size_t) and es (size_t).
*/

#ifndef _STDC___QSORT_HELPERS_H
#define _STDC___QSORT_HELPERS_H

#include <stddef.h>

#define __QSORT_MIN(a, b)       ((a) < (b) ? (a) : (b))

#define __QSORT_SWAPCODE(TYPE, parmi, parmj, n) {       \
        long i = (n) / sizeof(TYPE);                    \
        register TYPE *pi = (TYPE *)(parmi);            \
        register TYPE *pj = (TYPE *)(parmj);            \
        do {                                            \
                register TYPE t = *pi;                  \
                *pi++ = *pj;                            \
                *pj++ = t;                              \
        } while (--i > 0);                              \
}

#define __QSORT_SWAPINIT(a, es)                                         \
        swaptype = ((char *)(a) - (char *)0) % sizeof(long) ||          \
                   (es) % sizeof(long) ? 2 : (es) == sizeof(long) ? 0 : 1;

static inline void __qsort_swapfunc(char *a, char *b, int n, int swaptype)
{
        if (swaptype <= 1)
                __QSORT_SWAPCODE(long, a, b, n)
        else
                __QSORT_SWAPCODE(char, a, b, n)
}

#define __QSORT_SWAP(a, b)                              \
        if (swaptype == 0) {                            \
                long t = *(long *)(a);                  \
                *(long *)(a) = *(long *)(b);            \
                *(long *)(b) = t;                       \
        } else                                          \
                __qsort_swapfunc((a), (b), es, swaptype)

#define __QSORT_VECSWAP(a, b, n)  if ((n) > 0) __qsort_swapfunc((a), (b), (n), swaptype)

#endif /* _STDC___QSORT_HELPERS_H */

#ifdef QSORT_EMIT_BODY

/* Median of three, threading the comparison (and hence any context) through
   QSORT_CMP().  Like the original med3(), QSORT_CMP() may be evaluated more
   than once; comparator functions are expected to be free of side effects. */
#define __QSORT_MED3(a, b, c) (                                                 \
        QSORT_CMP((a), (b)) < 0                                                 \
          ? (QSORT_CMP((b), (c)) < 0 ? (b) : (QSORT_CMP((a), (c)) < 0 ? (c) : (a))) \
          : (QSORT_CMP((b), (c)) > 0 ? (b) : (QSORT_CMP((a), (c)) < 0 ? (a) : (c))))
{
        char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
        int d, r, swaptype, swap_cnt;

loop:   __QSORT_SWAPINIT(a, es);
        swap_cnt = 0;
        if (n < 7) {
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; pl > (char *)a && QSORT_CMP(pl - es, pl) > 0;
                             pl -= es)
                                __QSORT_SWAP(pl, pl - es);
                return;
        }
        pm = (char *)a + (n / 2) * es;
        if (n > 7) {
                pl = a;
                pn = (char *)a + (n - 1) * es;
                if (n > 40) {
                        d = (n / 8) * es;
                        pl = (char *)__QSORT_MED3(pl, pl + d, pl + 2 * d);
                        pm = (char *)__QSORT_MED3(pm - d, pm, pm + d);
                        pn = (char *)__QSORT_MED3(pn - 2 * d, pn - d, pn);
                }
                pm = (char *)__QSORT_MED3(pl, pm, pn);
        }
        __QSORT_SWAP(a, pm);
        pa = pb = (char *)a + es;

        pc = pd = (char *)a + (n - 1) * es;
        for (;;) {
                while (pb <= pc && (r = QSORT_CMP(pb, a)) <= 0) {
                        if (r == 0) {
                                swap_cnt = 1;
                                __QSORT_SWAP(pa, pb);
                                pa += es;
                        }
                        pb += es;
                }
                while (pb <= pc && (r = QSORT_CMP(pc, a)) >= 0) {
                        if (r == 0) {
                                swap_cnt = 1;
                                __QSORT_SWAP(pc, pd);
                                pd -= es;
                        }
                        pc -= es;
                }
                if (pb > pc)
                        break;
                __QSORT_SWAP(pb, pc);
                swap_cnt = 1;
                pb += es;
                pc -= es;
        }
        if (swap_cnt == 0) {  /* Switch to insertion sort */
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; pl > (char *)a && QSORT_CMP(pl - es, pl) > 0;
                             pl -= es)
                                __QSORT_SWAP(pl, pl - es);
                return;
        }

        pn = (char *)a + n * es;
        r = __QSORT_MIN(pa - (char *)a, pb - pa);
        __QSORT_VECSWAP(a, pb - r, r);
        r = __QSORT_MIN(pd - pc, pn - pd - es);
        __QSORT_VECSWAP(pb, pn - r, r);
        if ((r = pb - pa) > es)
                QSORT_RECURSE(a, r / es);
        if ((r = pd - pc) > es) {
                /* Iterate rather than recurse to save stack space */
                a = pn - r;
                n = r / es;
                goto loop;
        }
/*              QSORT_RECURSE(pn - r, r / es); */
}

#undef __QSORT_MED3

#endif /* QSORT_EMIT_BODY */
