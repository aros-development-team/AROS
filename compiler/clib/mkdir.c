/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function mkdir().
*/

#include <string.h>
#include <proto/dos.h>
#include <errno.h>
#include <stdlib.h>
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
    char *apath;

    if (!path) /*safety check */
    {
    	errno = EFAULT;
	return -1;
    }

    apath = (char*) __path_u2a(path);
    if (!apath)
        return -1;
        
    /* Duplicate apath to avoid modifying function argument if !__upath */
    apath = strdup(apath);
    if (!apath)
    {
        errno = ENOMEM;
        return -1;
    }

    /* Remove possible trailing / to avoid problems in handlers */
    if(strlen(apath) > 0 && apath[strlen(apath)-1] == '/')
	apath[strlen(apath)-1] = '\0';
    
    lock = CreateDir((STRPTR) apath);
    free(apath);

    if (!lock)
    {
    	errno = __arosc_ioerr2errno(IoErr());
	return -1;
    }

    UnLock(lock);

    return 0;
}

