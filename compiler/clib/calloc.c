/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function calloc()
    Lang: english
*/
#include <exec/types.h>

/*****************************************************************************

    NAME */
	#include <sys/types.h>
	#include <memory.h>

	void * calloc (

/*  SYNOPSIS */
	size_t count,
	size_t size)

/*  FUNCTION
	Allocate size bytes of memory, clears the memory (sets all bytes to
	0) and returns the address of the first byte.

    INPUTS
	count - How many time size
	size - How much memory to allocate.

    RESULT
	A pointer to the allocated memory or NULL. If you don't need the
	memory anymore, you can pass this pointer to free(). If you don't,
	the memory will be freed for you when the application exits.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	free(), malloc()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    ULONG * mem;

    /* Allocate the memory */
    mem = malloc (size*count);

    if (mem)
    {
	ULONG * ptr;

	ptr = mem;

	while (size > sizeof(ULONG))
	{
	    *ptr++ = 0;
	    size -= sizeof (ULONG);
	}

	if (size)
	{
	    UBYTE * bptr = (UBYTE *)ptr;

	    while (size --)
		*bptr ++ = 0;
	}
    }

    return mem;
} /* malloc */

