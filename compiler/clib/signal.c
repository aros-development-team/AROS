/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <stdio.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

	__sighandler_t *signal(

/*  SYNOPSIS */
	int sig,
	__sighandler_t *handler)

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
#   warning Implement signal()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");

    errno = ENOSYS;
    return (__sighandler_t *) -1;
}

