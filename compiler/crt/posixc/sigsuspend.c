/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigsuspend().
*/

#include "__posixc_intbase.h"

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
        Since AROS does not deliver asynchronous signals, this function
        sets the temporary mask, then immediately returns EINTR to
        avoid blocking forever.

    EXAMPLE

    BUGS

    SEE ALSO
        sigprocmask(), sigaction(), pause()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (!mask)
    {
        errno = EFAULT;
        return -1;
    }

    if (PosixCBase)
    {
        sigset_t oldmask;

        /* Save current mask, apply temporary mask */
        memcpy(&oldmask, &PosixCBase->sigmask, sizeof(sigset_t));
        memcpy(&PosixCBase->sigmask, mask, sizeof(sigset_t));

        /* Restore the original mask */
        memcpy(&PosixCBase->sigmask, &oldmask, sizeof(sigset_t));
    }

    /* sigsuspend always returns -1 with EINTR */
    errno = EINTR;
    return -1;

} /* sigsuspend */
