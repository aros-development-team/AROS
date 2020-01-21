/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <dos/dos.h>
#include <exec/types.h>

#include "dos_intern.h"

static AROS_UFH3(void, FreeFunc,
        AROS_UFHA(APTR, buffer, A1),
        AROS_UFHA(ULONG, length, D0),
        AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    FreeMem(buffer, length);

    AROS_USERFUNC_EXIT
}

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BOOL, UnLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR, seglist, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 26, Dos)

/*  FUNCTION
        Free a segment list allocated with LoadSeg().

    INPUTS
        seglist - The segment list.

    RESULT
        success = returns whether everything went ok. Returns FALSE if
                  seglist was NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        LoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    BOOL success = FALSE;

    if (seglist)
    {
        success = InternalUnLoadSeg(seglist, FreeFunc);
        if (success)
        {
            struct Node *segnode, *tmp;
            ObtainSemaphore(&((struct IntDosBase *)DOSBase)->segsem);
            ForeachNodeSafe(&((struct IntDosBase *)DOSBase)->segdata, segnode, tmp)
            {
                if (segnode->ln_Name == (char *)seglist)
                {
                    D(bug("[DOS] %s: freeing seglist info @ 0x%p\n", __func__, segnode);)
                    Remove(segnode);
                    FreeVec(segnode);
                    break;
                }
            }
            ReleaseSemaphore(&((struct IntDosBase *)DOSBase)->segsem);
        }
    }

    return success;

    AROS_LIBFUNC_EXIT
} /* UnLoadSeg */
