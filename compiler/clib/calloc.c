/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function calloc().
*/

#include <exec/types.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

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
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	free(), malloc()

    INTERNALS

******************************************************************************/
{
    ULONG * mem;

    size *= count;

    /* Allocate the memory */
    mem = malloc (size);

    if (mem)
	memset (mem, 0, size);

    return mem;
} /* calloc */

