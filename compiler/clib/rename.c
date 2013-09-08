/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function rename().
*/

#include <proto/dos.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "__upath.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int rename (

/*  SYNOPSIS */
	const char * oldpath,
	const char * newpath)

/*  FUNCTION
	Renames a file or directory.

    INPUTS
	oldpath - Complete path to existing file or directory.
	newpath - Complete path to the new file or directory.

    RESULT
	0 on success and -1 on error. In case of an error, errno is set.
	
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
          STRPTR aoldpath = (STRPTR)strdup((const char*)__path_u2a(oldpath));
    CONST_STRPTR anewpath = __path_u2a(newpath);
    BPTR oldlock, newlock;

    /* __path_u2a has resolved paths like /toto/../a */
    if (anewpath[0] == '.')
    {
	if (anewpath[1] == '\0' || (anewpath[1] == '.' && anewpath[2] == '\0'))
	{
	    errno = EEXIST;
	    free(aoldpath);
	    return -1;
	}
    }

    /* try to delete newpath first */
    Forbid();

    newlock = Lock(anewpath, SHARED_LOCK);
    if (newlock)
    {
	UnLock(newlock);

	oldlock = Lock(aoldpath, EXCLUSIVE_LOCK);
	if (oldlock)
	{
	    UnLock(oldlock);

	    /* DeleteFile returns an error if directory is non-empty */
	    if (!DeleteFile(anewpath))
	    {
		LONG ioerr = IoErr();
		errno = __stdc_ioerr2errno(ioerr);
		D(bug("rename(%s, %s) delete errno=%d, IoErr=%d\n",
			aoldpath, anewpath, errno, ioerr));
		free(aoldpath);
		Permit();
		return -1;
	    }
	}
    }

    if (!Rename (aoldpath, anewpath))
    {
	LONG ioerr = IoErr();
	errno = __stdc_ioerr2errno(ioerr);
	D(bug("rename(%s, %s) errno=%d, IoErr=%d\n",
		aoldpath, anewpath, errno, ioerr));
	free(aoldpath);
	Permit();
	return -1;
    }

    free(aoldpath);
    Permit();
    return 0;

} /* rename */

