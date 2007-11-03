/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function posix_memalign().
*/

#include "__arosc_privdata.h"
#include "__memalign.h"

#include <errno.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <sys/param.h>

/*****************************************************************************

    NAME */
#include <sys/types.h>
#include <stdlib.h>

	int posix_memalign (

/*  SYNOPSIS */
        void **memptr,
        size_t alignment,
        size_t size)

/*  FUNCTION
        Allocate aligned memory.

    INPUTS
        memptr - Pointer to a place to store the pointer to allocated memory.
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
        Memory allocated by posix_memalign() should be freed with free(). If
        not, it will be freed when the program terminates.

        This function must not be used in a shared library or in a threaded
	application.

        If an error occurs, errno will not be set.

    EXAMPLE

    BUGS

    SEE ALSO
	free()

    INTERNALS

******************************************************************************/
{
    UBYTE *mem = NULL, *orig;

    /* check the alignment is valid */
    if (alignment % sizeof(void *) != 0 || !powerof2(alignment))
        return EINVAL;

    /* allocate enough space to satisfy the alignment and save some info */
    mem = AllocPooled(__startup_mempool, size + alignment + AROS_ALIGN(sizeof(size_t)) + AROS_ALIGN(sizeof(void *)));
    if (mem == NULL)
        return ENOMEM;

    /* store the size for free(). it will add sizeof(size_t) itself */
    *((size_t *) mem) = size + alignment + AROS_ALIGN(sizeof(void *));
    mem += AROS_ALIGN(sizeof(size_t));

    /* if its already aligned correctly, then we just use it as-is */
    if (((IPTR) mem & (alignment-1)) == 0) {
        *memptr = mem;
        return 0;
    }

    orig = mem;

    /* move forward to an even alignment boundary */
    mem = (UBYTE *) (((IPTR) mem + alignment - 1) & -alignment);

    /* store a magic number in the place that free() will look for the
     * allocation size, so it can handle this specially */
    ((size_t *) mem)[-1] = MEMALIGN_MAGIC;

    /* then store the original pointer before it, for free() to find */
    ((void **) &(((size_t *) mem)[-1]))[-1] = orig;

    *memptr = mem;
    return 0;
} /* posix_memalign */
