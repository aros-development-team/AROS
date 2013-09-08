/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function posix_memalign().
*/

#include <errno.h>

/*****************************************************************************

    NAME */
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

        If an error occurs, errno will not be set.

    EXAMPLE

    BUGS

    SEE ALSO
	stdc.library/malloc_align(), stdc.library/calloc(),
        stdc.library/free(), stdc.library/malloc()

    INTERNALS

******************************************************************************/
{
    int ret = 0, old_errno;

    old_errno = errno;

    *memptr = malloc_align(size, alignment);
    if (!*memptr)
        ret = errno;

    errno = old_errno;

    return ret;
}
