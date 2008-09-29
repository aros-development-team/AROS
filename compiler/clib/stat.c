/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>

#include "__time.h"
#include "__errno.h"
#include "__stat.h"
#include "__upath.h"

/*****************************************************************************

    NAME */

#include <sys/stat.h>

	int stat(

/*  SYNOPSIS */
	const char *path,
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
    int res = 0;
    BPTR lock;

    path = __path_u2a(path);
    if (path == NULL)
        return -1;
	
    lock = Lock(path, SHARED_LOCK);
    if (!lock)
    {
	if  (IoErr() == ERROR_OBJECT_IN_USE)
	{
	    /* the file is already locked exclusively, so the only way to get
	       info about it is to find it in the parent directoy with the ExNext() function
            */

	    /* return an error for now */
	    errno = EACCES;
	    return -1;
        }

	errno = IoErr2errno(IoErr());
	return -1;
    }
    else
    res = __stat(lock, sb);

    UnLock(lock);

    return res;
}

