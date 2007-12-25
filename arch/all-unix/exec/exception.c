/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Exception - Perform a task exception.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/asmcall.h>

#include <signal.h>
#include <unistd.h>

/*****i*************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(void, Exception,

/*  LOCATION */
        struct ExecBase *, SysBase, 11, Exec)

/*  FUNCTION
        Exception handler. This function is called by the kernel if
        a task exception has occured. It is called in the Disable()'d
        state so that all signals are still unchanged.

        TF_EXCEPT is still set and must be reset by this routine.

        The user supplied exception code is called with the
        following parameters:

            D0  -   Mask of Flags which caused this exception.
            A1  -   Task->tc_ExceptData
            A6  -   SysBase

    INPUTS

    RESULT

    NOTES
        Exec internal function.

    EXAMPLE

    BUGS

    SEE ALSO
        Dispatch()

    INTERNALS
        At the end of this routine we set task->tc_State to TS_EXCEPT so the
        the kernel knows it can restore the signaled task context and set it
        to TS_RUN again.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task * task = FindTask(NULL);
    BYTE nestCnt;
    ULONG flags;

    task->tc_Flags &= ~TF_EXCEPT;

    nestCnt = SysBase->IDNestCnt;
    SysBase->IDNestCnt = 0;

    if ((flags = (task->tc_SigExcept & task->tc_SigRecvd)))
    {
        /* Call the Exception with the new AROS ASMCALL macros */
        if (task->tc_ExceptCode != NULL)
        {
            /* Block the signals we are handling now. The exception handler
               we are calling shall restore them.
            */
            task->tc_SigExcept ^= flags;

            Enable();
            task->tc_SigExcept |= AROS_UFC3(ULONG, task->tc_ExceptCode,
                AROS_UFCA(ULONG,             flags,               D0),
                AROS_UFCA(APTR,              task->tc_ExceptData, A1),
                AROS_UFCA(struct ExecBase *, SysBase,             A6)
            );
            Disable();
        }
        else
        {
            /* We don't have an exception handler, we shouldn't have
               any exceptions. Clear them.
            */

            task->tc_SigExcept = 0;
        }
    }

    task->tc_State = TS_EXCEPT;

    SysBase->IDNestCnt = nestCnt;
    
    /* Enter the kernel. We use an endless loop just in case the
       signal handler returns us to this point for whatever reason.
    */
    while (TRUE)
    {
        sigset_t temp_sig_int_mask;

        sigemptyset(&temp_sig_int_mask);
        sigaddset(&temp_sig_int_mask, SIGUSR1);
        sigprocmask(SIG_UNBLOCK, &temp_sig_int_mask, NULL);
        kill(getpid(), SIGUSR1);
    }

    AROS_LIBFUNC_EXIT
} /* Exception */
