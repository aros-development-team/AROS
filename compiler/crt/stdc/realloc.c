/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function realloc().
*/

#include <aros/cpu.h>
#include <proto/exec.h>

#include "__memalign.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        void * realloc (

/*  SYNOPSIS */
        void * oldmem,
        size_t size)

/*  FUNCTION
        Change the size of an allocated part of memory. The memory must
        have been allocated by malloc() or calloc(). If you reduce the
        size, the old contents will be lost. If you enlarge the size,
        the new contents will be undefined.

    INPUTS
        oldmem - What you got from malloc() or calloc().
        size - The new size.

    RESULT
        A pointer to the allocated memory or NULL. If you don't need the
        memory anymore, you can pass this pointer to free(). If you don't,
        the memory will be freed for you when the application exits.

    NOTES
        If you get NULL, the memory at oldmem will not have been freed and
        can still be used.

    EXAMPLE

    BUGS

    SEE ALSO
        calloc(), free(), malloc()

    INTERNALS

******************************************************************************/
{
    UBYTE * mem, * newmem;
    size_t oldsize;

    if (!oldmem)
        return malloc (size);

    /* C allows realloc(ptr, 0) to free ptr; do so and return NULL, which is
       the behaviour most callers expect (and matches the historical idiom). */
    if (size == 0)
    {
        free (oldmem);
        return NULL;
    }

    mem = (UBYTE *)oldmem - MALLOC_HDRSIZE;
    oldsize = *((size_t *)mem);

    /* A block from aligned_alloc()/posix_memalign() has the magic number in
       place of its size; what is usable is the rest of the malloc() block it
       lies in. Always move it, as the result need not keep its alignment. */
    if (oldsize == MEMALIGN_MAGIC)
    {
        UBYTE *orig = ((UBYTE **)(mem - AROS_ALIGN(sizeof(void *))))[0];

        oldsize = *((size_t *)(orig - MALLOC_HDRSIZE)) - ((UBYTE *)oldmem - orig);
        goto copy;
    }

    /* Reduce or enlarge the memory ? */
    if (size < oldsize)
    {
        /* Don't change anything for small changes */
        if ((oldsize - size) < 4096)
            newmem = oldmem;
        else
            goto copy;
    }
    else if (size == oldsize) /* Keep the size ? */
        newmem = oldmem;
    else
    {
copy:
        newmem = malloc (size);

        if (newmem)
        {
            if (size > oldsize)
                size = oldsize;
            CopyMem (oldmem, newmem, size);
            free (oldmem);
        }
    }

    return newmem;
} /* realloc */

