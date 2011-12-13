/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a seglist for ROM code.
*/

#include <aros/debug.h>
#include <proto/exec.h>

struct phony_segment
{
    ULONG Size;	/* Length of segment in # of bytes */
    IPTR  Next;	/* Next segment (always 0 for this) */
    struct FullJumpVec Code;	/* Code to jump to the offset */
} __attribute__((packed));

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

        dos.library/UnloadSeg()

    INTERNALS

*****************************************************************************/
{
    struct phony_segment *segtmp;

    segtmp = AllocMem(sizeof(*segtmp), MEMF_ANY);
    if (!segtmp)
    	return BNULL;

    segtmp->Size = sizeof(*segtmp);
    segtmp->Next = (IPTR)0;
    __AROS_SET_FULLJMP(&segtmp->Code, function);

    CacheClearE(&segtmp->Code, sizeof(struct FullJumpVec), CACRF_ClearI | CACRF_ClearD);

    D(bug("[CreateSegList] Created jump segment 0x%p, code 0x%p, target 0x%p\n", MKBADDR(&segtmp->Next), &segtmp->Code, function));

    return MKBADDR(&segtmp->Next);
}
