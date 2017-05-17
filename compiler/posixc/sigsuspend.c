/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function sigsuspend().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

	int sigsuspend (

/*  SYNOPSIS */
	const sigset_t *mask)

/*  FUNCTION
        replace the callers signal mask, and suspend it
        until it signaled to terminate, or to invoke a
        signal handler.

        If the signal terminates the process, sigsuspend()
        doesn't return.

        If the signal is caught, sigsuspend() returns following the
        signal handler, and the signal mask is restored to
        the state prior to calling sigsuspend(). 

        SIGKILL or SIGSTOP cannot be blocked; specifying
        them in the mask has no effect on the process's signal mask. 

    INPUTS

    RESULT
        always returns -1, normally with the error EINTR.

    NOTES
        Not implemented.

        Normally used in conjunction with sigprocmask(), to prevent
        signal delivery during critical code sections. Callers must 
        block the signals with sigprocmask(). On completion, the caller
        waits for signals by calling sigsuspend() with the return value
        of sigprocmask()

    EXAMPLE

    BUGS

    SEE ALSO
        kill(), pause(), sigaction(), signal(), sigprocmask(),
        sigwaitinfo(), sigsetops(), sigwait()

    INTERNALS
        POSIX.1-2001

******************************************************************************/
{
    /* TODO: Implement sigsuspend() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");

    errno = ENOSYS;

    return -1;

} /* sigsuspend */
