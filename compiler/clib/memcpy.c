/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function memcpy()
    Lang: english
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <string.h>

	void * memcpy (

/*  SYNOPSIS */
	void *	     dest,
	const void * src,
	size_t	     count)

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
	memmove()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    AROS_GET_SYSBASE
    CopyMem ((UBYTE *)src, dest, count);

    return dest;
} /* memcpy */

