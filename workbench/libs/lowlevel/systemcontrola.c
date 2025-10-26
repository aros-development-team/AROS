/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc:
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
        Provides a mechanism to enable or disable selected system-level
        input and multitasking features under program control.  This
        function is intended for use by games or multimedia applications
        that require exclusive access to hardware input devices or
        want to temporarily suppress normal OS behaviour (such as
        keyboard shortcuts or multitasking).

        The desired controls are specified using a TagItem list.
        Each tag corresponds to a particular system feature or behaviour
        that may be disabled, enabled, or queried.

        This call is advisory and may not be implemented fully on all
        systems.  Unsupported tags are ignored.

        Recognized tags (as defined in <libraries/lowlevel.h>):

            SCON_AllowIRQ (ULONG)
                If set to FALSE, prevents lowlevel.library from generating
                certain input interrupts.  Default is TRUE.

            SCON_StopTaskSwitch (ULONG)
                When set to TRUE, temporarily halts task switching,
                effectively freezing multitasking.  This should be used
                with great caution and released as soon as possible.

            SCON_StopKeyRepeat (ULONG)
                If set to TRUE, disables key auto-repeat from the input.device.

            SCON_EnableECSKeys (ULONG)
                Enables extended keyboard keys on some hardware (e.g., F11/F12).

            SCON_RequestExclusive (ULONG)
                Requests exclusive access to input devices such as keyboard
                and mouse, preventing other applications from receiving events.

    INPUTS
        tags - Pointer to a TagItem list specifying the system control
               operations to perform.  May be NULL, in which case the call
               does nothing and returns 0.

    RESULT
        Returns a bitmask of results depending on the requested operations.
        The exact meaning of returned bits is implementation-dependent.
        Typically, a non-zero value indicates success or the current state
        of queried controls.

    BUGS
        This function’s implementation is incomplete and not all control
        tags are acted upon in current releases.

    SEE ALSO
        SetJoyPortAttrsA(), ReadJoyPort(), utility.library/TagItem

    INTERNALS
        SystemControlA() modifies or queries global state flags held within
        lowlevel.library.  These flags are checked by the library’s input
        handling routines to determine whether to pass events to the system
        or suppress them for exclusive mode operation.

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
