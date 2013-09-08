/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function pclose().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int pclose(

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO
	popen()

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement pclose() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    errno = ENOSYS;

    return -1;
} /* pclose() */

