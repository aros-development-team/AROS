/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:46  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:59  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:07  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/types.h>

	AROS_LH3I(void, CopyMemQuick,

/*  SYNOPSIS */
	AROS_LHA(APTR,  source, A0),
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

    HISTORY
	22-10-95    Created by M. Fleischer

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG low,high;

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
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	while(--low);

    /*
	Partly unrolled copying loop. The predecrement helps the compiler to
	find the best possible loop. The if is necessary to do this.
    */
    if(high)
	do
	{
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	    *((ULONG *)dest)++=*((ULONG *)source)++;
	}while(--high);
    AROS_LIBFUNC_EXIT
} /* CopyMemQuick */

