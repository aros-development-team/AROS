/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function memchr().
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <string.h>

	void * memchr (

/*  SYNOPSIS */
	const void * mem,
	int	     c,
	size_t	     n)

/*  FUNCTION
	Copy the contents of a part of memory to another. Both areas
	must not overlap. If they do, use memmove().

    INPUTS
	dest - The first byte of the destination area in memory
	src - The first byte of the source area in memory
	count - How many bytes to copy

    RESULT
	dest.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    const char * ptr = (char *)mem;

    while (n)
    {
	if (*ptr == c)
	    return ((void *)ptr);

	n --;
	ptr ++;
    }

    return NULL;
} /* memchr */

