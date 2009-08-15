/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <dos/filesystem.h>
#include <proto/dos.h>

#include <errno.h>

#include "__arosc_privdata.h"
#include "__errno.h"
#include "__filesystem_support.h"
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	ssize_t readlink(

/*  SYNOPSIS */
        const char *path,
	char       *buf,
	size_t      bufsize)

/*  FUNCTION
        Places the contents of a symbolic link in a buffer of given size. No NUL
	char is appended to the buffer.

    INPUTS
        path    - the path to the symbolic link  
	buf     - pointer to the buffer where to store the symbolic link content
	bufsize - the size of the buffer in bytes
      
    RESULT
        The call returns the count of characters placed in the buffer if it
        succeeds, or a -1 if an error occurs, placing the error code in the
	global variable errno.
*/
{
    ssize_t          res = -1;
    struct IOFileSys iofs;
    struct DevProc   *dvp = NULL;
    LONG             error;
    struct Process   *me = (struct Process *)FindTask(NULL);

    /* check for empty path before potential conversion from "." to "" */
    if (__doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return res;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return res;

    /* we need to know if it is really a soft link */
    InitIOFS(&iofs, FSA_OPEN, DOSBase);
    iofs.io_Union.io_OPEN.io_FileMode = FMF_READ;
    iofs.io_Union.io_OPEN.io_Filename = StripVolume(path);

    do
    {
        if ((dvp = GetDeviceProc(path, dvp)) == NULL)
        {
            error = IoErr();
            break;
        }

        error = DoIOFS(&iofs, dvp, NULL, DOSBase);
    }
    while (error == ERROR_OBJECT_NOT_FOUND);

    if (error == ERROR_NO_MORE_ENTRIES)
        error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;
    else if (error == 0)
    {
        /* open I/O request successful, but this is not*/
        /* what we want, don't forget to close again */
        InitIOFS(&iofs, FSA_CLOSE, DOSBase);
        DoIO((struct IORequest *) &iofs);
        /* set an error that translates to EINVAL */
        error = me->pr_Result2 = ERROR_OBJECT_WRONG_TYPE;
    }
    else if (error == ERROR_IS_SOFT_LINK)
    {
        /* it is a soft link, try to read it */
        res = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, path, buf, bufsize);
        if (res == -1)
            error = IoErr();
        else
        {
            if (res == -2)
                res = bufsize;
            
            /* clear the soft link error */
            error = me->pr_Result2 = 0;
        }
    }
    
    FreeDeviceProc(dvp);

    if (error)
        errno = IoErr2errno(error);

    return res;
}
