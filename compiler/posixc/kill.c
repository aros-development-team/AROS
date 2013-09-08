/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function kill().
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <signal.h>

	int kill (

/*  SYNOPSIS */
	pid_t pid,
        int   sig)

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
    /* TODO: Implement kill() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = ENOSYS;

    return -1;
} /* kill() */

