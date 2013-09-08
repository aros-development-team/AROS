/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function realloc().
*/

#include <aros/cpu.h>
#include <proto/exec.h>

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

    mem = (UBYTE *)oldmem - AROS_ALIGN(sizeof(size_t));
    oldsize = *((size_t *)mem);

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

