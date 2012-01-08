/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function link().
*/

#include <errno.h>

/*****************************************************************************

    NAME */

#include <unistd.h>

	int link(

/*  SYNOPSIS */
	const char *oldpath,
	const char *newpath)

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
} /* link */

