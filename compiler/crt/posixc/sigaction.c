/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigaction().
*/

#include <aros/debug.h>
#include <string.h>
#include <errno.h>

/* Access stdc internals for signal_func_data */
#include "../stdc/__signal.h"

/*****************************************************************************

    NAME */
#include <signal.h>

        int sigaction (

/*  SYNOPSIS */
        int signum,
        const  struct  sigaction  *act,
        struct sigaction *oldact)

/*  FUNCTION
        Examine and change a signal action.

    INPUTS
        signum - signal number to examine/change (must not be SIGKILL or
                 SIGSTOP)
        act    - if non-NULL, set the signal action to this value
        oldact - if non-NULL, store the previous signal action here

    RESULT
        0 on success, -1 on error with errno set.

    NOTES
        Supported sa_flags: SA_SIGINFO, SA_RESETHAND, SA_NODEFER, SA_RESTART.
        SA_ONSTACK, SA_NOCLDSTOP, SA_NOCLDWAIT are accepted but have no
        effect since AROS does not deliver asynchronous signals.

    EXAMPLE

    BUGS

    SEE ALSO
        signal(), sigprocmask(), raise()

    INTERNALS
        Uses the shared signal_func_data array in StdCIntBase so that
        signal() and sigaction() stay synchronized.

******************************************************************************/
{
    struct signal_func_data *sigfuncdata;

    /* Validate signal number */
    if (signum < 1 || signum == SIGKILL || signum == SIGSTOP)
    {
        errno = EINVAL;
        return -1;
    }

    sigfuncdata = __stdc_sig_getfuncdata(signum);
    if (!sigfuncdata)
    {
        errno = EINVAL;
        return -1;
    }

    /* Return old action if requested */
    if (oldact)
    {
        memset(oldact, 0, sizeof(struct sigaction));
        if (sigfuncdata->sa_flags & SA_SIGINFO)
        {
            oldact->sa_sigaction =
                (void (*)(int, siginfo_t *, void *))sigfuncdata->sigaction_func;
            oldact->sa_flags = sigfuncdata->sa_flags;
        }
        else
        {
            oldact->sa_handler = sigfuncdata->sigfunc;
            oldact->sa_flags = sigfuncdata->sa_flags;
        }
        memcpy(&oldact->sa_mask, &sigfuncdata->sa_mask, sizeof(sigset_t));
    }

    /* Install new action if provided */
    if (act)
    {
        sigfuncdata->sa_flags = act->sa_flags;
        memcpy(&sigfuncdata->sa_mask, &act->sa_mask, sizeof(sigset_t));

        if (act->sa_flags & SA_SIGINFO)
        {
            sigfuncdata->sigaction_func =
                (void (*)(int, void *, void *))act->sa_sigaction;
            sigfuncdata->sigfunc = (void *)act->sa_sigaction;
        }
        else
        {
            sigfuncdata->sigfunc = act->sa_handler;
            sigfuncdata->sigaction_func = NULL;
        }
    }

    return 0;
} /* sigaction */
