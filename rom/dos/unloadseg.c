/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

    if (seglist)
    {
#if AROS_MODULES_DEBUG
        extern struct MinList debug_seglist;
        extern struct MinList free_debug_segnodes;
        struct debug_segnode *segnode;

    	Forbid();
        ForeachNode(&debug_seglist, segnode)
        {
            if (segnode->seglist == seglist)
            {
		/* use the same free function as loadseg ! */
		struct seginfo *si;
		while ((si = (struct seginfo *)REMHEAD(&segnode->seginfos)))
		{
		    FreeMem(si, sizeof(struct seginfo));
		}

                REMOVE(segnode);
		ADDHEAD(&free_debug_segnodes, segnode);
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
