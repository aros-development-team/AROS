/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    AROS extension function malloc_align().
*/

#include <exec/types.h>

#include "__memalign.h"

#include <errno.h>

#define powerof2(x) ((((x)-1)&(x))==0)

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void *malloc_align (

/*  SYNOPSIS */
        size_t size,
        size_t alignment)

/*  FUNCTION
        Allocate aligned memory.

    INPUTS
	size - How much memory to allocate.
        alignment - Alignment of allocated memory. The address of the
                    allocated memory will be a multiple of this value, which
                    must be a power of two and a multiple of sizeof(void *).

    RESULT
        A pointer to the allocated memory or NULL.

    NOTES
        errno is set to EINVAL if the alignment parameter was not a power of
        two, or was not a multiple of sizeof(void *).
        errno is set to ENOMEM if there was insufficient memory to fulfill
        the request.
        Memory allocated by malloc_align() should be freed with free(). If
        not, it will be freed when the program terminates.

        This function is AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO
	calloc(), free(), malloc()

    INTERNALS

******************************************************************************/
{
    char *mem = NULL, *orig, *tmp;

    /* check the alignment is valid */
    if (alignment % sizeof(void *) != 0 || !powerof2(alignment))
    {
        errno = EINVAL;
        return NULL;
    }

    /* allocate enough space to satisfy the alignment and save some info */
    mem = malloc(size + alignment + AROS_ALIGN(sizeof(size_t)) + AROS_ALIGN(sizeof(void *)));
    if (mem == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    /* if it's already aligned correctly, then we just use it as-is */
    if (((IPTR) mem & (alignment-1)) == 0)
        return mem;

    orig = mem;

    /* move forward to an even alignment boundary */
    mem = (char *) (((IPTR) mem + alignment - 1) & -alignment);
    tmp = mem;

    /* store a magic number in the place that free() will look for the
     * allocation size, so it can handle this specially */
    tmp -= AROS_ALIGN(sizeof(size_t *));
    *((size_t *) tmp) = MEMALIGN_MAGIC;

    /* then store the original pointer before it, for free() to find */
    tmp -= sizeof(void *);
    *((void **) tmp) = orig;

    return mem;
} /* posix_memalign */
