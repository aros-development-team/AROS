/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

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
        The behaviour of raise() follows the BSD semantics.
        For each signal the system keeps track of a signal handler is already
        being called.
        If not, the signal handler is called; when yes this will logged and the
        handler will be recalled when the first handler returns. If the a new
        handler is registered that one will be used then.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct signal_func_data *sigfuncdata = __sig_getfuncdata(signum);

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

        if (func != SIG_IGN)
            func(signum);

        sigfuncdata->flags &= ~__SIG_RUNNING; /* Signal is not running anymore */
    }
    
    return 0;
}
