/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function rename().
*/
#include <proto/dos.h>
#include <errno.h>

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
    BPTR oldlock, newlock;

    /* try to delete newpath first */
    Forbid();

    newlock = Lock(newpath, SHARED_LOCK);
    if (newlock)
    {
	UnLock(newlock);

	oldlock = Lock(oldpath, EXCLUSIVE_LOCK);
	if (oldlock)
	{
	    UnLock(oldlock);

	    /* DeleteFile returns an error if directory is non-empty */
	    if (!DeleteFile(newpath))
	    {
                LONG ioerr = IoErr();
                errno = __stdc_ioerr2errno(ioerr);
		D(bug("rename(%s, %s) delete errno=%d, IoErr=%d\n",
			oldpath, newpath, errno, ioerr));
		Permit();
		return -1;
	    }
	}
    }

    if (!Rename (oldpath, newpath))
    {
        LONG ioerr = IoErr();
        errno = __stdc_ioerr2errno(ioerr);
	D(bug("rename(%s, %s) errno=%d, IoErr=%d\n",
		oldpath, newpath, errno, ioerr));
	Permit();
	return -1;
    }

    Permit();
    return 0;

} /* rename */
