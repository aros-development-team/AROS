/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level debugging support.
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/arossupport.h>

        BPTR CreateSegList(

/*  SYNOPSIS */
        APTR function )

/*  LOCATION */

/*  FUNCTION
        Create a SegList, which contains a call to 'function'

    INPUTS
        function - Function to call when the SegList is executed

    RESULT
        BPTR to the SegList that was allocated. This SegList can
             be freed by DOS/UnloadSeg. If not enough memory,
             BNULL will be returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        UnloadSeg()

    INTERNALS

*****************************************************************************/
{
    struct {
	ULONG Size;	/* Length of segment in # of bytes */
	IPTR  Next;	/* Next segment (always 0 for this) */
	struct FullJumpVec Code;	/* Code to jump to the offset */
    } *segtmp;

    segtmp = AllocMem(sizeof(*segtmp), MEMF_CLEAR);
    if (!segtmp)
    	return BNULL;

    segtmp->Size = sizeof(*segtmp);
    segtmp->Next = (IPTR)0;
    __AROS_SET_FULLJMP(&segtmp->Code, function);

    return MKBADDR(&segtmp->Next);
}
