/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/cpu.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	void * realloc_nocopy (

/*  SYNOPSIS */
	void * oldmem,
	size_t size)

/*  FUNCTION
	Change the size of an allocated part of memory. The memory must
	have been allocated by malloc(), calloc(), realloc() or realloc_nocopy().
        
        The reallocated buffer, unlike with realloc(), is not guaranteed to hold
        a copy of the old one.

    INPUTS
	oldmen - What you got from malloc(), calloc(), realloc() or realloc_nocopy().
                 If NULL, the function will behave exactly like malloc().
	size   - The new size. If 0, the buffer will be freed.

    RESULT
	A pointer to the allocated memory or NULL. If you don't need the
	memory anymore, you can pass this pointer to free(). If you don't,
	the memory will be freed for you when the application exits.

    NOTES
	If you get NULL, the memory at oldmem will not have been freed and
	can still be used.

        This function is AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO
	free(), malloc(), calloc(), realloc()

    INTERNALS

******************************************************************************/
{
    UBYTE * mem, * newmem;
    size_t oldsize;

    if (!oldmem)
	return malloc (size);

    mem = (UBYTE *)oldmem - AROS_ALIGN(sizeof(size_t));
    oldsize = *((size_t *)mem);

    /* Reduce or enlarge the memory ? */
    if (size < oldsize)
    {
	/* Don't change anything for small changes */
	if ((oldsize - size) < 4096)
	    newmem = oldmem;
	else
	    goto alloc;
    }
    else if (size == oldsize) /* Keep the size ? */
	newmem = oldmem;
    else
    {
alloc:
        newmem = malloc (size);

	if (newmem)
	    free (oldmem);
    }

    return newmem;
} /* realloc */

