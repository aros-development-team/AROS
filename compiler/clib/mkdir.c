/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function mkdir().
*/

#include <proto/dos.h>
#include <errno.h>
#include "__errno.h"
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <sys/stat.h>

	int mkdir (

/*  SYNOPSIS */
	const char *path,
	mode_t      mode)

/*  FUNCTION
	 Make a directory file

    INPUTS
	path - the path of the directory being created
	mode - the permission flags for the directory

    RESULT
	0 on success or -1 on errorr.

    NOTES


    EXAMPLE

    BUGS

    SEE ALSO
	chmod(),  stat(),  umask()

    INTERNALS

******************************************************************************/

{
    BPTR lock;

    if (!path) /*safety check */
    {
    	errno = EFAULT;
	return -1;
    }

    path = __path_u2a(path);
    if (!path)
        return -1;
	    
    lock = CreateDir(path);

    if (!lock)
    {
    	errno = IoErr2errno(IoErr());
	return -1;
    }

    UnLock(lock);

    return 0;
}

