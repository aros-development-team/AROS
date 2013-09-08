/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

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
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;
    
    return -1;
} /* sigprocmask */
