/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function read()
    Lang: english
*/
#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__stdio.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	ssize_t read (

/*  SYNOPSIS */
	int    fd,
	void * buf,
	size_t count)

/*  FUNCTION
	Read an amount of bytes from a file descriptor.

    INPUTS
	fd - The file descriptor to read from
	buf - The buffer to read the bytes into
	count - Read this many bytes.

    RESULT
	The number of characters read (may range from 0 when the file
	descriptor contains no more characters to count) or -1 on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	open(), read(), fread()

    INTERNALS

    HISTORY
	15.12.1996 digulla created

******************************************************************************/
{
    ssize_t cnt;
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    cnt = Read ((BPTR)fdesc->fh, buf, count);

    if (cnt == -1)
	errno = IoErr2errno (IoErr ());

    return cnt;
} /* read */

