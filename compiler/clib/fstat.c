/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function fstat().
*/

#include <errno.h>

#include "__stat.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

	int fstat(

/*  SYNOPSIS */
	int fd,
	struct stat *sb)

/*  FUNCTION
	Returns information about a file specified by an open file descriptor.
	Information is stored in stat structure. Consult stat() documentation
	for detailed description of that structure.

    INPUTS
	filedes - File descriptor of the file
	sb - Pointer to stat structure that will be filled by the fstat()
	call.

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	stat()

    INTERNALS
	Consult stat() documentation for details.

******************************************************************************/
{
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;

	return -1;
    }

    return __stat(desc->fcb->fh, sb, TRUE);
}

