/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function write()
    Lang: english
*/
#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__errno.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	ssize_t write (

/*  SYNOPSIS */
	int	     fd,
	const void * buf,
	size_t	     count)

/*  FUNCTION
	Write an amount of characters to the specified file descriptor.

    INPUTS
	fd - The file descriptor to write to
	buf - Write these bytes into the file descriptor
	count - Write that many bytes

    RESULT
	The number of characters written or -1 on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	06.12.1996 digulla created

******************************************************************************/
{
    ssize_t cnt;

    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    cnt = Write ((BPTR)fdesc->fh, (void *)buf, count);

    if (cnt == -1)
	errno = IoErr2errno (IoErr ());

    return cnt;
} /* write */

