/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function kill().
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
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;
    
    return -1;
} /* kill() */

