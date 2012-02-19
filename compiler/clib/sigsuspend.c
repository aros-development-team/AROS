/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function sigsuspend().
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
    /* TODO: Implement sigsuspend() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return -1;
} /* sigsuspend */
