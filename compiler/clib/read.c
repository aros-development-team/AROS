/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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

    switch (fd)
    {
    case 0: /* Stdin */
	cnt = Read (Input(), buf, count);
	break;

    case 1: /* Stdout */
    case 2: /* Stderr */
	errno = EINVAL;
	return EOF;

    default: {
	FILENODE * fn;

	fn = GetFilenode4fd (fd);

	if (!fn)
	{
	    errno = EBADF;
	    return EOF;
	}

	cnt = Read ((BPTR)fn->File.fh, buf, count);

	break; }
    }

    if (cnt == -1)
	errno = IoErr2errno (IoErr ());

    return cnt;
} /* read */

