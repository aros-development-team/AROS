/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>
#include <proto/arossupport.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <libraries/lowlevel.h>

#include "lowlevel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/lowlevel.h>

      AROS_LH1(ULONG, SystemControlA,

/*  SYNOPSIS */ 
      AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 12, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This functions implementation is incomplete.

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tagp = tags;;
    Tag failtag = 0;

    D(bug("[lowlevel] %s()\n", __func__);)

    /* For now, dump all tags in debug mode */
    while ((tag = LibNextTagItem(&tagp))) {
        switch (tag->ti_Tag)
        {
        case SCON_TakeOverSys:
                if (tag->ti_Data)
                    Forbid();
                else
                    Permit();
                break;
        case SCON_KillReq:
                {
                    struct Process *thisProc = (struct Process *)FindTask(NULL);
                    if (thisProc->pr_Task.tc_Node.ln_Type == NT_PROCESS)
                    {
                        if (tag->ti_Data)
                            thisProc->pr_WindowPtr = (APTR)-1;
                        else
                            thisProc->pr_WindowPtr = (APTR)0;
                    }
                }
                break;

        case SCON_CDReboot:
        case SCON_StopInput:
        case SCON_RemCreateKeys:
        default:
                D(bug("%s: Tag SCON_Dummy+%d, Data %p\n", __func__, tag->ti_Tag - SCON_Dummy, (APTR)tag->ti_Data));
                failtag = tag->ti_Tag;
                break;
        }
    }

    return failtag;

    AROS_LIBFUNC_EXIT
} /* SystemControlA */
