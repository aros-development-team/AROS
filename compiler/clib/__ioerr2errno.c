/*
    (C) 1995-96 AROS - The Amiga Research OS
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
    }

    return MAX_ERRNO+1;
} /* IoErr2errno */
