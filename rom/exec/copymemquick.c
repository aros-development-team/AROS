#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/types.h>

	__AROS_LH3I(void, CopyMemQuick,

/*  SYNOPSIS */
	__AROS_LA(APTR,  source, A0),
	__AROS_LA(APTR,  dest,   A1),
	__AROS_LA(ULONG, size,   D0),

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
    __AROS_FUNC_INIT

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
    __AROS_FUNC_EXIT
} /* CopyMemQuick */

