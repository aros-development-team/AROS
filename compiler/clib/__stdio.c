/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: stdio internals
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE

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
#include "__open.h"
#include "__stdio.h"

FILE * stdin;
FILE * stdout;
FILE * stderr;

struct MinList __stdio_files =
{
    (struct MinNode *)&__stdio_files.mlh_Tail,
    NULL,
    (struct MinNode *)&__stdio_files
};


int __smode2oflags(char *mode)
{
    int ret = -1;

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

    if ( *mode == '+' || (*mode == 'b' && mode[1] == '+'))
    {
	ret = O_RDWR | (ret & ~O_ACCMODE);
    }
    else if (*mode)
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

void __init_stdio(void)
{
    if
    (
        !(stdin  = fdopen(STDIN_FILENO, NULL))  ||
    	!(stdout = fdopen(STDOUT_FILENO, NULL)) ||
    	!(stderr = fdopen(STDERR_FILENO, NULL))
    )
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	exit(20);
    }
}

ADD2INIT(__init_stdio, 5);


