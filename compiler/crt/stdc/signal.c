/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
*/

#include <string.h>

#include "__signal.h"

/*****************************************************************************

    NAME */

#include <signal.h>

        __sighandler_t *signal(

/*  SYNOPSIS */
        int signum,
        __sighandler_t *handler)

/*  FUNCTION
        Set signal handler for a signal.

    INPUTS
        signum - the signal number to register a handler for
        handler - the signal handler; can be SIG_IGN, SIG_DFL or a function
                  pointer that will handle the signal

    RESULT
        The old handler that was replaced by the new handler.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        sigaction()

    INTERNALS
        When signal() is used, SA_SIGINFO mode and sa_mask are cleared
        so that the handler is treated as a plain signal(2)-style handler.

******************************************************************************/
{
    struct signal_func_data *sigfuncdata = __stdc_sig_getfuncdata(signum);

    if (!sigfuncdata)
        return SIG_ERR;

    __sighandler_t *ret = sigfuncdata->sigfunc;
    sigfuncdata->sigfunc = handler;
    sigfuncdata->sigaction_func = NULL;
    sigfuncdata->sa_flags = 0;
    memset(&sigfuncdata->sa_mask, 0, sizeof(sigset_t));

    return ret;
}
