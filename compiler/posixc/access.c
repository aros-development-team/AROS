/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function access().
*/

#include <errno.h>
#include <proto/dos.h>
#include <string.h>

#include <aros/debug.h>

#include "__upath.h"
#include "__posixc_intbase.h"

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

    EXAMPLE

    BUGS

    SEE ALSO
	open(), ftruncate()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    BPTR lock = BNULL;
    struct FileInfoBlock *fib = NULL;
    int result = -1;
    char vol[32];
    struct DosList *dl = NULL;
    const char *apath;

    if (!path) /* safety check */
    {
    	errno = EFAULT;
        return -1;
    }

    D(bug("[access] Path: %s\n", path));
    if (!strlen(path)) /* empty path */
    {
        errno = ENOENT;
        return -1;
    }

    /* POSIX root is (poorly) emulated, its contents is accessible */
    if (PosixCBase->doupath && (path[0] == '/') && (path[1] == '\0'))
    {
	if (mode & (W_OK|R_OK)) {
	    errno = EACCES;
	    return -1;
	} else
	    return 0;
    }

    apath = __path_u2a(path);
    D(bug("[access] AROS path: %s\n", apath));
    /* Check if the volume exists. Calling Lock on non-existing volume will bring up System Requester */
    if (SplitName(apath, ':', vol, 0, sizeof(vol)-1) != -1)
    {
	D(bug("[access] Volume name: %s\n", vol));
	if(strcmp(vol, "PROGDIR") != 0)
	{
            dl = LockDosList(LDF_ALL | LDF_READ);
            dl = FindDosEntry(dl, vol, LDF_ALL);
            UnLockDosList(LDF_ALL | LDF_READ);
            /* Volume / Assign / Device not found */
            if (dl == NULL)
            {
                errno = ENOENT;
                return -1;
            }
	}
    }

    /* Create a lock and examine a lock */

    lock = Lock(apath, SHARED_LOCK);
    if (lock == BNULL)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        errno = __stdc_ioerr2errno(IoErr());
        UnLock(lock);
        return -1;
    }
        
    if (Examine(lock, fib))
    {
        /* Notice : protection flags are 'low-active' (0 means access is granted) */
        result = 0;
        if ((mode & R_OK) && (result == 0) && (fib->fib_Protection & (1 << FIBB_READ)))
        {
            errno = EACCES;
            result = -1;
        }
        if ((mode & W_OK) && (result == 0) && (fib->fib_Protection & (1 << FIBB_WRITE)))
        {
            errno = EACCES;
            result = -1;
        }
        if ((mode & X_OK) && (result == 0) && (fib->fib_Protection & (1 << FIBB_EXECUTE)))
        {
            errno = EACCES;
            result = -1;
        }
    }
    else
    {
	/* We get here if Examine() failed. However it can be a character device
	   (NIL:, ZERO:, etc) which does not support EXAMINE action.
	   Currently we consider them read-write. If really needded, the routine
	   can be modified in order to try to open the device in different modes. */
	BOOL ischar = FALSE;
	BPTR fh = OpenFromLock(lock);

	if (fh)
	{
	    ischar = IsInteractive(fh);

	    Close(fh);
	    lock = BNULL;
	}

	if (ischar)
	{
	    if (mode & X_OK)
	    {
		/* Character devices are not executable in any way */
		errno = EACCES;
		result = -1;
	    }
	    else
		result = 0;
	}
	else
	{
            errno = EBADF;
            result = -1;
	}
    }

    FreeDosObject(DOS_FIB, fib);
    if (lock);
	UnLock(lock);

    return result;
}
