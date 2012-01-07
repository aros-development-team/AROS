/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function readlink().
*/

#include <aros/debug.h>

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
    struct aroscbase *aroscbase = __GM_GetBase();
    ssize_t          res = -1;
    struct DevProc   *dvp = NULL;
    LONG             error;
    struct Process   *me = (struct Process *)FindTask(NULL);

    /* check for empty path before potential conversion from "." to "" */
    if (aroscbase->acb_doupath && path && *path == '\0')
    {
        errno = ENOENT;
        return res;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return res;

    res = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, path, buf, bufsize);
    if (res == -1) {
        error = IoErr();
    } else {
        if (res == -2)
            res = bufsize;
        error = me->pr_Result2 = 0;
    }
    
    FreeDeviceProc(dvp);

    if (error)
        errno = IoErr2errno(error);

    return res;
}
