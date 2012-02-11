/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

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
        Implemented but no interrupts will be generated like when pressing
        Ctrl-C; signal handlers can for now only be called by raise() in the
        program itself.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct signal_func_data *sigfuncdata = __sig_getfuncdata(signum);

    if (!sigfuncdata)
        return SIG_ERR;

    __sighandler_t *ret = sigfuncdata->sigfunc;
    sigfuncdata->sigfunc = handler;

    return ret;
}

