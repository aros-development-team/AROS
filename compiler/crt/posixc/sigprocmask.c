/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sigprocmask().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

        int sigprocmask (

/*  SYNOPSIS */
        int  how,
        const  sigset_t *set,
        sigset_t *oldset)

/*  FUNCTION
        Allow the caller to examine or change (or both) the
        signal mask of the calling thread.

    INPUTS

    RESULT

    NOTES
        Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement sigprocmask() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = ENOSYS;
    
    return -1;
} /* sigprocmask */
