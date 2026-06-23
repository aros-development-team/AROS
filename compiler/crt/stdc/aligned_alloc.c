/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    C11 function aligned_alloc().
*/

#include <exec/types.h>

#include "__memalign.h"

#include <errno.h>
#include <stdint.h>

#define powerof2(x) ((((x)-1)&(x))==0)

/*****************************************************************************

    NAME */
#include <stdlib.h>

        void *aligned_alloc (

/*  SYNOPSIS */
        size_t alignment,
        size_t size)

/*  FUNCTION
        Allocate aligned memory.

    INPUTS
        alignment - Alignment of allocated memory. The address of the
                    allocated memory will be a multiple of this value, which
                    must be a power of two and a multiple of sizeof(void *).
        size - How much memory to allocate.

    RESULT
        Returns zero on success.
        Returns EINVAL if the alignment parameter was not a power of two, or
        was not a multiple of sizeof(void *).
        Returns ENOMEM if there was insufficient memory to fulfill the request.

    NOTES
        Memory allocated by aligned_alloc() should be freed with free(). If
        not, it will be freed when the program terminates.

        If an error occurs, errno will not be set.

    EXAMPLE

    BUGS

    SEE ALSO
        calloc(), free(), malloc()

    INTERNALS

******************************************************************************/
{
    char *mem = NULL, *orig, *tmp;
    size_t extra;

    /* check the alignment is valid: it must be non-zero, a power of two and
       a multiple of sizeof(void *). Note powerof2(0) is true, so 0 has to be
       rejected explicitly (it would also divide-by-zero the boundary maths). */
    if (alignment == 0 || (alignment % sizeof(void *)) != 0 || !powerof2(alignment))
    {
        errno = EINVAL;
        return NULL;
    }

    /* Guard the bookkeeping arithmetic against size_t overflow before asking
       malloc() for the padded block. */
    extra = alignment + AROS_ALIGN(sizeof(size_t)) + AROS_ALIGN(sizeof(void *));
    if (size > SIZE_MAX - extra)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* allocate enough space to satisfy the alignment and save some info */
    orig = malloc(size + extra);
    if (orig == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* if it's already aligned correctly, then we just use it as-is */
    if (((IPTR) orig & (alignment-1)) == 0)
        return orig;

    /* move forward to an even alignment boundary */
    mem = (char *) (((IPTR) orig + AROS_ALIGN(sizeof(size_t)) + AROS_ALIGN(sizeof(void *)) + alignment - 1) & -alignment);
    tmp = mem;

    /* store a magic number in the place that free() will look for the
     * allocation size, so it can handle this specially */
    tmp -= AROS_ALIGN(sizeof(size_t *));
    *((size_t *) tmp) = MEMALIGN_MAGIC;

    /* then store the original pointer before it, for free() to find */
    tmp -= AROS_ALIGN(sizeof(void *));
    *((void **) tmp) = orig;

    return mem;
}
