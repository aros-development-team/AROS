/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigprocmask().
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
