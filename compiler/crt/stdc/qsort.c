/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function qsort().
*/
/* Original source from NetBSD */
#include <stdlib.h>

/* The Bentley & McIlroy quicksort algorithm is shared with qsort_r(); see
   <__qsort.h>.  This first include provides the swap helpers at file scope. */
#include "__qsort.h"

/*****************************************************************************

    NAME */

        void qsort (

/*  SYNOPSIS */
        void * a,
        size_t n,
        size_t es,
        int (* cmp)(const void *, const void *))

/*  FUNCTION
        Sort the array a. It contains n elements of the size es. Elements
        are compares using the function cmp().

    INPUTS
        a - The array to sort
        n - The number of elements in the array
        es - The size of a single element in the array
        cmp - The function which is called when two elements must be
                compared. The function gets the addresses of two elements
                of the array and must return 0 is both are equal, < 0 if
                the first element is less than the second and > 0 otherwise.

    RESULT
        None.

    NOTES

    EXAMPLE
        // Use this function to compare to stringpointers
        int cmp_strptr (const char ** sptr1, const char ** sptr2)
        {
            return strcmp (*sptr1, *sptr2);
        }

        // Sort an array of strings
        char ** strings;

        // fill the array
        strings = malloc (sizeof (char *)*4);
        strings[0] = strdup ("h");
        strings[1] = strdup ("a");
        strings[2] = strdup ("f");
        strings[3] = strdup ("d");

        // Sort it
        qsort (strings, sizeof (char *), 4, (void *)cmp_strptr);

    BUGS

    SEE ALSO
        qsort_r(), strcmp(), strncmp(), memcmp(), strcasecmp(), strncasecmp()

    INTERNALS
        The algorithm itself lives in <__qsort.h> and is shared with qsort_r();
        here the comparator is called with just the two elements.

******************************************************************************/
{
#define QSORT_CMP(x, y)         cmp((const void *)(x), (const void *)(y))
#define QSORT_RECURSE(b, num)   qsort((b), (num), es, cmp)
#define QSORT_EMIT_BODY
#include "__qsort.h"
#undef QSORT_EMIT_BODY
#undef QSORT_RECURSE
#undef QSORT_CMP
}
