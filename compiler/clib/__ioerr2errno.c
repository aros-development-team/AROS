/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: errno internals
    Lang: english
*/

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/filesystem.h>
#include "__errno.h"

int IoErr2errno (int ioerr)
{
    switch (ioerr)
    {
	case 0:
	    return 0;

	case ERROR_OBJECT_WRONG_TYPE:
	    return EINVAL;

	case ERROR_NO_FREE_STORE:
	    return ENOMEM;

	case ERROR_OBJECT_NOT_FOUND:
	    return ENOENT;

        case ERROR_WOULD_BLOCK:
	    return EAGAIN;

        case ERROR_BROKEN_PIPE:
	    return EPIPE;

	case ERROR_OBJECT_EXISTS:
	    return EEXIST;

	case ERROR_BUFFER_OVERFLOW:
	    return ENOBUFS;

	case ERROR_BREAK:
	    return EINTR;

	case ERROR_FILE_NOT_OBJECT:
	case ERROR_NOT_EXECUTABLE:
	    return ENOEXEC;

	case ERROR_OBJECT_IN_USE:
	    return EBUSY;

	case ERROR_DIR_NOT_FOUND:
	    return ENOTDIR;
    }

    return MAX_ERRNO+ioerr;
} /* IoErr2errno */
