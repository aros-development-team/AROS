/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <string.h>
#include <aros/types/sigaction_s.h>

#include "__signal.h"

/*****************************************************************************

    NAME */
#include <signal.h>

        int raise(

/*  SYNOPSIS */
        int signum)

/*  FUNCTION
        Calls the handler of a signal

    INPUTS
        Signal handler to be called.

    RESULT
        0: OK
        -1: error calling handler, errno will be set.

    NOTES
        The behavior of raise() follows the BSD semantics.
        For each signal the system keeps track of a signal handler is already
        being called.
        If not, the signal handler is called; when yes this will logged and the
        handler will be recalled when the first handler returns. If the a new
        handler is registered that one will be used then.

        When sigaction() has been used to register a handler the following
        flags are honored:
        SA_SIGINFO  - call sa_sigaction(signum, NULL, NULL) instead of sa_handler
        SA_RESETHAND - reset to SIG_DFL after one delivery
        SA_NODEFER  - don't block the signal during its own handler

    EXAMPLE

    BUGS

    SEE ALSO
        signal(), sigaction()

    INTERNALS

******************************************************************************/
{
    struct signal_func_data *sigfuncdata = __stdc_sig_getfuncdata(signum);

    if (!sigfuncdata)
        return -1;

    /* If a signal handler raises it's own signal then the current
       signal handler will be called when the first handler completes.
    */
    sigfuncdata->flags |= __SIG_PENDING;
    while (!(sigfuncdata->flags & __SIG_RUNNING)
           && (sigfuncdata->flags & __SIG_PENDING)
    )
    {
        __sighandler_t *func = sigfuncdata->sigfunc;
        if (func == SIG_DFL)
            func = __sig_default;

        sigfuncdata->flags |= __SIG_RUNNING; /* Signal handler is running ... */
        sigfuncdata->flags &= ~__SIG_PENDING; /* ... and not pending anymore */

        /* Save flags and sigaction handler before potential SA_RESETHAND */
        int saved_sa_flags = sigfuncdata->sa_flags;
        void (*saved_sigaction_func)(int, void *, void *) =
            sigfuncdata->sigaction_func;

        /* SA_RESETHAND: reset handler to SIG_DFL after first delivery */
        if (saved_sa_flags & SA_RESETHAND)
        {
            sigfuncdata->sigfunc = SIG_DFL;
            sigfuncdata->sigaction_func = NULL;
            sigfuncdata->sa_flags = 0;
        }

        if (func != SIG_IGN)
        {
            if ((saved_sa_flags & SA_SIGINFO) && saved_sigaction_func)
            {
                /* SA_SIGINFO mode: call three-argument handler */
                saved_sigaction_func(signum, NULL, NULL);
            }
            else
            {
                func(signum);
            }
        }

        sigfuncdata->flags &= ~__SIG_RUNNING; /* Signal is not running anymore */
    }
    
    return 0;
}
