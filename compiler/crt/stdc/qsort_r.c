/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    GNU/BSD extension qsort_r() (standardised in POSIX.1-2024).
*/
#include <stdlib.h>

/* The Bentley & McIlroy quicksort algorithm is shared with qsort(); see
   <__qsort.h>.  This first include provides the swap helpers at file scope. */
#include "__qsort.h"

/*****************************************************************************

    NAME */

        void qsort_r (

/*  SYNOPSIS */
        void * a,
        size_t n,
        size_t es,
        int (* cmp)(const void *, const void *, void *),
        void * arg)

/*  FUNCTION
        Sort the array a, which contains n elements of size es, exactly like
        qsort(), except that the comparison function cmp() takes a third
        argument.  The pointer arg is passed unchanged as that third argument
        on every call, letting the comparison capture context without using
        global variables (so it is reentrant and thread safe).

    INPUTS
        a - The array to sort
        n - The number of elements in the array
        es - The size of a single element in the array
        cmp - The comparison function.  It is called with the addresses of two
                elements and the arg pointer, and must return 0 if both are
                equal, < 0 if the first element is less than the second and
                > 0 otherwise.
        arg - An arbitrary pointer passed through to every cmp() call.

    RESULT
        None.

    NOTES
        This implementation uses the GNU/glibc argument order: the context
        pointer is the last parameter of qsort_r() and the last parameter of
        the comparison function (compar(a, b, arg)).

    EXAMPLE
        // Sort integers relative to a pivot supplied via arg.
        int cmp_rel (const void *a, const void *b, void *arg)
        {
            int pivot = *(int *)arg;
            int da = abs(*(const int *)a - pivot);
            int db = abs(*(const int *)b - pivot);
            return da - db;
        }

        int pivot = 5;
        qsort_r (array, count, sizeof(int), cmp_rel, &pivot);

    BUGS

    SEE ALSO
        qsort(), strcmp(), strncmp(), memcmp()

    INTERNALS
        The algorithm itself lives in <__qsort.h> and is shared with qsort();
        here the comparator is additionally passed the arg pointer.

******************************************************************************/
{
#define QSORT_CMP(x, y)         cmp((const void *)(x), (const void *)(y), arg)
#define QSORT_RECURSE(b, num)   qsort_r((b), (num), es, cmp, arg)
#define QSORT_EMIT_BODY
#include "__qsort.h"
#undef QSORT_EMIT_BODY
#undef QSORT_RECURSE
#undef QSORT_CMP
}
