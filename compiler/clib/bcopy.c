/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: bcopy()
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <string.h>

	void bcopy (

/*  SYNOPSIS */
	const void * src,
	void *       dst,
	size_t       len)

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

    SEE ALSO
	memmove(), exec/CopyMem()

    INTERNALS

    HISTORY
	22-10-96    ldp created

******************************************************************************/
{
    CopyMem ((UBYTE *)src, dst, len);

} /* bcopy */

