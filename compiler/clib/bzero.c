/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2001 function bzero().
    Function is deprecated and removed from POSIX.1-2008
*/

#include <proto/exec.h>

#define bzero bzero_inline
#include <strings.h>
#undef bzero

/*****************************************************************************

    NAME */
#include <strings.h>

	void bzero (

/*  SYNOPSIS */
	void * ptr,
	size_t    len)

/*  FUNCTION
	Write len zero bytes to ptr. If len is zero, does nothing.

    INPUTS
	ptr - The first byte of the area in memory to be cleared.
	len - How many bytes to clear.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    UBYTE * bptr = ptr;

    while (((IPTR)bptr)&(AROS_LONGALIGN-1) && len)
    {
	*bptr ++ = 0;
	len --;
    }

    if (len > sizeof(ULONG))
    {
	ULONG * ulptr = (ULONG *)bptr;

	while (len > sizeof(ULONG))
	{
	    *ulptr ++ = 0UL;
	    len -= sizeof(ULONG);
	}
	
	bptr = (UBYTE *)ulptr;
    }

    while (len --)
	*bptr ++ = 0;

} /* bzero */

