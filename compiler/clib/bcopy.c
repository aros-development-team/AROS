/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function bcopy()
    Lang: english
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <string.h>

	void bcopy (

/*  SYNOPSIS */
	const void * src,
	void *	     dst,
	int	     len)

/*  FUNCTION
	Copy the contents of a part of memory to another. Both areas
	must not overlap. If they do, use memmove().

    INPUTS
	src - The first byte of the source area in memory
	dst - The first byte of the destination area in memory
	len - How many bytes to copy.

    RESULT

    NOTES
	The original bcopy() allows overlapping src and dst.

    EXAMPLE

    BUGS
	Overlapping memory areas are not supported. This should be fixed.

    SEE ALSO
	memmove(), exec/CopyMem()

    INTERNALS

    HISTORY
	22-10-96    ldp created

******************************************************************************/
{
    (void) memmove (dst, src, len);
} /* bcopy */

