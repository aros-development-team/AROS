/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <dos/dos.h>
#include <exec/types.h>
#include "dos_intern.h"

extern void Exec_FreeMem();

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
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)


    if (seglist)
    {
#ifdef AROS_MODULES_DEBUG
        extern struct MinList debug_seglist;
        struct debug_segnode *segnode;

    	Forbid();
        ForeachNode(&debug_seglist, segnode)
        {
            if (segnode->seglist == seglist)
            {
                REMOVE(segnode);
                FreeMem(segnode, sizeof(struct debug_segnode));

                break;
            }
        }
	Permit();
#endif
        return InternalUnLoadSeg(seglist, __AROS_GETVECADDR(SysBase, 35));
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* UnLoadSeg */
