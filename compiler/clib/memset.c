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

	void * memset (

/*  SYNOPSIS */
	void * dest,
	int    c,
	size_t count)

/*  FUNCTION
	Fill the memory at dest with count times c.

    INPUTS
	dest - The first byte of the destination area in memory
	c - The byte to fill memory with
	count - How many bytes to write

    RESULT
	dest.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	memmove(), memcpy()

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    UBYTE * ptr = dest;

    while (((IPTR)ptr)&(AROS_LONGALIGN-1) && count)
    {
	*ptr ++ = c;
	count --;
    }

    if (count > sizeof(ULONG))
    {
	ULONG * ulptr = (ULONG *)ptr;
	ULONG fill;

	fill = (ULONG)(c & 0xFF);
	fill = (fill <<  8) | fill;
	fill = (fill << 16) | fill;

	while (count > sizeof(ULONG))
	{
	    *ulptr ++ = fill;
	    count -= sizeof(ULONG);
	}

        ptr = (UBYTE *)ulptr;
    }

    while (count --)
	*ptr ++ = c;

    return dest;
} /* memset */

