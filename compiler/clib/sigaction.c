/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigaction().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int sigaction (

/*  SYNOPSIS */
	int signum,
	const  struct  sigaction  *act,
	struct sigaction *oldact)

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
    /* TODO: Implement sigaction() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;
    
    return -1;
} /* sigaction */
