/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy aligned memory.
    Lang: english
*/
#include <aros/libcall.h>
#include <exec/types.h>

/*****************************************************************************

    NAME */

	AROS_LH3I(void, CopyMemQuick,

/*  SYNOPSIS */
	AROS_LHA(CONST_APTR,  source, A0),
	AROS_LHA(APTR,  dest,   A1),
	AROS_LHA(ULONG, size,   D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 105, Exec)

/*  FUNCTION
	Copy some longwords from one destination in memory to another using
	a fast copying method.

    INPUTS
	source - Pointer to source area (must be ULONG aligned)
	dest   - Pointer to destination (must be ULONG aligned)
	size   - number of bytes to copy (must be a multiple of sizeof(ULONG))

    RESULT

    NOTES
	The source and destination area are not allowed to overlap.

    EXAMPLE

    BUGS

    SEE ALSO
	CopyMem()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG low,high;
    const ULONG *src = source;
    ULONG *dst = dest;

    /* Calculate number of ULONGs to copy */
    size/=sizeof(ULONG);

    /*
	To minimize the loop overhead I copy more than one (eight) ULONG per
	iteration. Therefore I need to split size into size/8 and the rest.
    */
    low =size&7;
    high=size/8;

    /* Then copy for both parts */
    if(low)
	do
	    *dst++=*src++;
	while(--low);

    /*
	Partly unrolled copying loop. The predecrement helps the compiler to
	find the best possible loop. The if is necessary to do this.
    */
    if(high)
	do
	{
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	    *dst++=*src++;
	}while(--high);
    AROS_LIBFUNC_EXIT
} /* CopyMemQuick */

