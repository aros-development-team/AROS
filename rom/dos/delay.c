/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/


#include <proto/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"
#include <devices/timer.h>
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(void, Delay,

/*  SYNOPSIS */
        AROS_LHA(ULONG, timeout, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 33, Dos)

/*  FUNCTION
        Waits for at least the time specified as timeout.

    INPUTS
        timeout - the minimum time to wait in ticks (1/50 seconds)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct timerequest  timerio;
    struct MsgPort      timermp;

    memset(&timermp, 0, sizeof(timermp));
    
    timermp.mp_Node.ln_Type = NT_MSGPORT;
    timermp.mp_Flags        = PA_SIGNAL;
    timermp.mp_SigBit       = SIGB_SINGLE;
    timermp.mp_SigTask      = FindTask(NULL);    
    NEWLIST(&timermp.mp_MsgList);
    
    /* clone timerequest in DOSBase */
    CopyMem(DOSBase->dl_TimeReq, &timerio, sizeof(timerio));

    timerio.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    timerio.tr_node.io_Message.mn_ReplyPort    = &timermp;    
    timerio.tr_node.io_Command                 = TR_ADDREQUEST;
    timerio.tr_time.tv_secs                    = timeout / TICKS_PER_SECOND;
    timerio.tr_time.tv_micro                   = 1000000UL / TICKS_PER_SECOND * (timeout % TICKS_PER_SECOND);

    SetSignal(0, SIGF_SINGLE);
        
    DoIO(&timerio.tr_node);
    
    AROS_LIBFUNC_EXIT
} /* Delay */
