/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
    BPTR    fh;
    ssize_t cnt;

    switch (fd)
    {
    case 0: /* Stdin */
	errno = EINVAL;
	return EOF;

    case 1: /* Stdout */
	fh = Output();

    case 2: { /* Stderr */
	struct Process * me = (struct Process *)FindTask (NULL);

	fh = me->pr_CES ? me->pr_CES : me->pr_COS;

	break; }

    default: {
	FILENODE * fn;

	fn = GetFilenode4fd (fd);

	if (!fn)
	{
	    errno = EINVAL;
	    return EOF;
	}

	fh = (BPTR)fn->File.fh;

	break; }
    }

    cnt = Write (fh, (void *)buf, count);

    if (cnt == EOF)
	errno = IoErr2errno (IoErr ());

    return cnt;
} /* write */

