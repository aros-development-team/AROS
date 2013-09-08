/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function mknod().
*/

#include <aros/debug.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <sys/stat.h>

	int mknod(

/*  SYNOPSIS */
	const char *pathname,
	mode_t mode,
	dev_t dev)

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
    /* TODO: Implement mknod() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = EPERM;
    return -1;
} /* mknod */

