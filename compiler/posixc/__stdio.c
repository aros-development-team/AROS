/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stdio internals
    Lang: English
*/

#include "__arosc_privdata.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <exec/lists.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include "__stdio.h"

int __smode2oflags(const char *mode)
{
    int ret = -1;
    int theresb = 0;

    switch (*mode++)
    {
	case 'r':
	    ret = O_RDONLY;
	    break;

	case 'w':
	    ret = O_WRONLY | O_CREAT | O_TRUNC;
	    break;

	case 'a':
	    ret = O_WRONLY | O_CREAT | O_APPEND;
	    break;

	default:
	    errno = EINVAL;
	    return -1;
    }

    if (*mode == 'b')
    {
	theresb = 1;
	mode++;
    }
    else if (*mode == 't')
    {
	// Silently ignore 't' (=text).
	// It's deprecated, but it's still in some sources,
	// and on other platforms they can compiled without problems. 
	mode++;
    }

    if (*mode == '+')
    {
	ret = O_RDWR | (ret & ~O_ACCMODE);
    	mode++;
    }

    if (*mode == 'b' && !theresb)
    {
	mode++;
    }

    if (*mode != '\0')
    {
    	errno = EINVAL;
	return -1;
    }

    return ret;
}

int __oflags2sflags(int omode)
{
    int ret;

    switch (omode & O_ACCMODE)
    {
    	case O_RDONLY:
	    ret = _STDIO_READ;
    	    break;

	case O_WRONLY:
	    ret = _STDIO_WRITE;
	    break;

	case O_RDWR:
	    ret = _STDIO_READ | _STDIO_WRITE;
	    break;

        default:
	    errno = EINVAL;
	    return 0;
    }

    if (omode & O_APPEND)
    	ret |= _STDIO_APPEND;

    return ret;
}

int __init_stdio(struct aroscbase *aroscbase)
{
    NEWLIST(&aroscbase->acb_stdio_files);

    if
    (
        !(stdin  = fdopen(STDIN_FILENO, NULL))  ||
    	!(stdout = fdopen(STDOUT_FILENO, NULL)) ||
    	!(stderr = fdopen(STDERR_FILENO, NULL))
    )
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return 0;
    }

    return 1;
}

ADD2OPENLIB(__init_stdio, 5);


