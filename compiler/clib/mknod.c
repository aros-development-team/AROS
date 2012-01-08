/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function mknod().
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*****************************************************************************

    NAME */

	int mknod(

/*  SYNOPSIS */
	const char *pathname,
	mode_t mode,
	dev_t dev)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    errno = EPERM;
    return -1;
} /* mknod */

