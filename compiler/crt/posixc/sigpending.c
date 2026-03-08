/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigpending().
*/

#include <string.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

        int sigpending (

/*  SYNOPSIS */
        sigset_t *set)

/*  FUNCTION
        Return the set of signals that are pending for delivery.

    INPUTS
        set - receives the set of pending signals

    RESULT
        0 on success, -1 on error with errno set.

    NOTES
        Since AROS does not deliver asynchronous signals, the pending
        set is always empty.

    EXAMPLE

    BUGS

    SEE ALSO
        sigprocmask(), sigaction()

    INTERNALS

******************************************************************************/
{
    if (!set)
    {
        errno = EFAULT;
        return -1;
    }

    /* No asynchronous signal delivery on AROS, so nothing is pending */
    memset(set, 0, sizeof(sigset_t));

    return 0;
} /* sigpending */
