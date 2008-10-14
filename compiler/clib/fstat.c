/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function fstat().
*/

#include <errno.h>

#include "__stat.h"
#include "__open.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

	int fstat(

/*  SYNOPSIS */
	int fd,
	struct stat *sb)

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
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;

	return -1;
    }

    return __stat(desc->fcb->fh, sb);
}

