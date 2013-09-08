/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function sigpending().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

	int sigpending (

/*  SYNOPSIS */
	sigset_t *set)

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
    /* TODO: Implement sigpending() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;
    
    return -1;
} /* sigpending */
