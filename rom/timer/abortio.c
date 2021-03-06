/*
    Copyright (C) 1995-2007, The AROS Development Team. All rights reserved.

    Desc: AbortIO() - abort a running timer request.
*/
#include "timer_intern.h"
#include <exec/io.h>
#include <exec/errors.h>

/*****i***********************************************************************

    NAME */
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/execlock.h>
#include <proto/timer.h>

        AROS_LH1(LONG, AbortIO,

/*  SYNOPSIS */
        AROS_LHA(struct timerequest *, timereq, A1),

/*  LOCATION */
        struct TimerBase *, TimerBase, 6,Timer)

/*  FUNCTION
        Abort a running timer.device request.

    INPUTS
        timereq     -   The request you wish to abort.

    RESULT
        0   if the request was aborted, io_Error will also be set
            to the value IOERR_ABORTED.

        -1  otherwise (most likely that the request isn't working).

        If the request is successfully aborted, you should WaitIO() on
        the message before you try and reuse it.

    NOTES
        This function may be called from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
        exec/AbortIO(), exec/WaitIO()

    INTERNALS

    HISTORY
        18-02-1997  iaint   Implemented.

******************************************************************************/
{
    AROS_LIBFUNC_INIT
#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase = TimerBase->tb_ExecLockBase;
#endif
    LONG ret = -1;

    /*
        As the timer.device runs as an interrupt, we had better protect
        the "waiting timers" list from being corrupted.
    */

    Disable();
#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase) ObtainLock(TimerBase->tb_ListLock, SPINLOCK_MODE_WRITE, 0);
#endif
    if(timereq->tr_node.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
    {
        Remove((struct Node *)timereq);

        timereq->tr_node.io_Error = IOERR_ABORTED;
        timereq->tr_time.tv_secs = 0;
        timereq->tr_time.tv_micro = 0;

        if (!(timereq->tr_node.io_Flags & IOF_QUICK))
            ReplyMsg((struct Message *)timereq);
        ret = 0;
    }
#if defined(__AROSEXEC_SMP__)
    if (ExecLockBase) ReleaseLock(TimerBase->tb_ListLock, 0);
#endif
    Enable();

    return ret;

    AROS_LIBFUNC_EXIT
} /* AbortIO */
