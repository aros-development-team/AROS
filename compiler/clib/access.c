/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function access().
*/

#include <errno.h>
#include <proto/dos.h>
#include <dos/filesystem.h>

#include "__errno.h"
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int access (

/*  SYNOPSIS */
	const char *path,
	int         mode)

/*  FUNCTION
	Check access permissions of a file or pathname

    INPUTS
	path - the path of the file being checked
	mode - the bitwise inclusive OR of the access permissions
	       to be checked:

	       W_OK - for write permission
	       R_OK - for readpermissions
	       X_OK - for execute permission
	       F_OK - Just to see whether the file exists

    RESULT
	If path cannot be found or if any of the desired access
	modes would not be granted, then a -1 value is returned;
	otherwise a 0 value is returned.

    NOTES
	Even if a process has appropriate privileges and indicates
	success for X_OK, the file may not actually have execute
	permission bits set.  Likewise for R_OK and W_OK.

    EXAMPLE

    BUGS

    SEE ALSO
	open(), ftruncate()

    INTERNALS

******************************************************************************/
{
    ULONG amode = 0;
    BPTR fh;

    if (!path) /* safety check */
    {
    	errno = EFAULT;
	return -1;
    }

    /* how can we check whether a file exists without having read permission?? */
    if (!mode) mode = R_OK;

    if (mode & R_OK) amode |= FMF_READ;
    if (mode & W_OK) amode |= FMF_WRITE;
    if (mode & X_OK) amode |= FMF_EXECUTE;

    if (!(fh = Open(__path_u2a(path), amode)))
    {
	errno = IoErr2errno(IoErr());
	return -1;
    }

    Close(fh);
    return 0;
}
