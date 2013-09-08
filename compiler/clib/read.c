/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function read().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"

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

******************************************************************************/
{
    ssize_t cnt;
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    if(fdesc->fcb->privflags & _FCB_ISDIR)
    {
	errno = EISDIR;
	return -1;
    }

    cnt = Read ((BPTR)fdesc->fcb->fh, buf, count);

    if (cnt == -1)
	errno = __stdc_ioerr2errno (IoErr ());

    return cnt;
} /* read */

