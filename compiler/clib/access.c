/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function access().
*/

#include <errno.h>
#include <proto/dos.h>
#include <dos/filesystem.h>
#include <string.h>

#include <aros/debug.h>

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

    EXAMPLE

    BUGS

    SEE ALSO
	open(), ftruncate()

    INTERNALS

******************************************************************************/
{
    BPTR lock = NULL;
    struct FileInfoBlock *fib = NULL;
    int result = -1;
    char vol[32];
    struct DosList *dl = NULL;

    if (!path) /* safety check */
    {
    	errno = EFAULT;
        return -1;
    }

    if (!strlen(path)) /* empty path */
    {
        errno = ENOENT;
        return -1;
    }

    /* Check if the volume exists. Calling Lock on non-existing volume will bring up System Requester */
    if (SplitName(__path_u2a(path), ':', vol, 0, sizeof(vol)-1) != -1)
    {
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

    lock = Lock(__path_u2a(path), SHARED_LOCK);
    if (lock == NULL)
    {
        errno = IoErr2errno(IoErr());
        return -1;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        errno = IoErr2errno(IoErr());
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
        errno = EBADF;
        result = -1;
    }

    FreeDosObject(DOS_FIB, fib);
    fib = NULL;

    UnLock(lock);
    return result;
}
