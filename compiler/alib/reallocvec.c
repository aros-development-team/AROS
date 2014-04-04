/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: New Exec pendant of ANSI C function realloc() using AllocVec()
    Lang: english
*/

#define REALLOC_MININCREASE 1024
#define REALLOC_MINDECREASE 4096

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	APTR ReAllocVec (

/*  SYNOPSIS */
	APTR  oldmem,
	ULONG newsize,
	ULONG requirements)

/*  FUNCTION
	Change the size of an AllocVec:ed part of memory. The memory must
	have been allocated by AllocVec(). If you reduce the
	size, the old contents will be lost. If you enlarge the size,
	the new contents will be undefined.

    INPUTS
	oldmen - What you got from AllocVec().
	newsize - The new size.
	requirements - The (new) requirements.
		Note that if no new block of memory is allocated, the
		requirements are not considered.

    RESULT
	A pointer to the allocated memory or NULL. If you don't need the
	memory anymore, you can pass this pointer to FreeVec().

    NOTES
	If you get NULL, the memory at oldmem will not have been freed and
	can still be used.
	Note that if no new block of memory is allocated, the requirements
	are not considered.

        This function must not be used in a shared library or in a
	threaded application. (???)


    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/AllocVec(), exec.library/FreeVec(), exec.library/CopyMem()

    INTERNALS

    HISTORY

******************************************************************************/
{
//    AROS_LIBFUNC_INIT
    UBYTE * mem, * newmem;
    ULONG oldsize;

    if (!oldmem)
	return AllocVec (newsize, requirements);

    mem = (UBYTE *)oldmem - AROS_ALIGN(sizeof(IPTR));
    oldsize = *((ULONG *)mem) - AROS_ALIGN(sizeof(IPTR));

    /* Reduce or enlarge the memory ? */
    if (newsize < oldsize)
    {
	/* Don't change anything for small changes */
	if ((oldsize - newsize) < REALLOC_MINDECREASE)
	    newmem = oldmem;
	else
	    goto copy;
    }
    else if (newsize == oldsize) /* Keep the size ? */
	newmem = oldmem;
    else
    {
	/* It is likely that if memory is ReAllocVec:ed once it will
	   be ReAllocVec:ed again, so don't be too stingy with memory */
	if ((newsize - oldsize) < REALLOC_MININCREASE)
	    newsize = oldsize + REALLOC_MININCREASE;
copy:
	newmem = AllocVec(newsize, requirements);

	if (newmem)
	{
	    CopyMem (oldmem, newmem, newsize);
	    FreeVec (oldmem);
	}
    }

    return newmem;
//    AROS_LIBFUNC_EXIT
} /* ReAllocVec */
