/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigsuspend().
*/

#include "__posixc_intbase.h"

#include <proto/exec.h>
#include <dos/dos.h>

#include <string.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

        int sigsuspend (

/*  SYNOPSIS */
        const sigset_t *mask)

/*  FUNCTION
        Replace the callers signal mask temporarily, and suspend the
        process until a signal is delivered that either terminates it
        or invokes a signal handler.

    INPUTS
        mask - the temporary signal mask to use while suspended

    RESULT
        Always returns -1 with errno set to EINTR (successful return
        after a signal handler) or EFAULT (bad pointer).

    NOTES
        AROS has no asynchronous POSIX signal delivery. To still provide a
        genuine suspend (rather than returning immediately), this blocks on
        the task's CTRL-C break signal and translates a received break into
        SIGINT when SIGINT is not blocked by the temporary mask.

    EXAMPLE

    BUGS

    SEE ALSO
        sigprocmask(), sigaction(), pause()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    sigset_t oldmask;

    if (!mask)
    {
        errno = EFAULT;
        return -1;
    }

    if (!PosixCBase)
    {
        errno = EINVAL;
        return -1;
    }

    /* Atomically install the temporary signal mask. */
    memcpy(&oldmask, &PosixCBase->sigmask, sizeof(sigset_t));
    memcpy(&PosixCBase->sigmask, mask, sizeof(sigset_t));

    /* Suspend until the task is broken with CTRL-C. Deliver SIGINT (under the
       temporary mask) when it is not blocked, then return. A CTRL-C always
       ends the wait so the call can never hang indefinitely. */
    for (;;)
    {
        ULONG sigs = Wait(SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C)
        {
            if (!sigismember(&PosixCBase->sigmask, SIGINT))
                raise(SIGINT);
            break;
        }
    }

    /* Restore the previous mask before returning. */
    memcpy(&PosixCBase->sigmask, &oldmask, sizeof(sigset_t));

    /* sigsuspend always returns -1 with EINTR */
    errno = EINTR;
    return -1;

} /* sigsuspend */
