/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function write()
    Lang: english
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
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
    AROS_GET_SYSBASE_OK

    ssize_t cnt;

    fdesc *fdesc = __getfdesc(fd);
    kprintf( "clib/write: entering\n");
    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    cnt = Write ((BPTR)fdesc->fh, (void *)buf, count);

    if (cnt == -1)
	errno = IoErr2errno (IoErr ());

    kprintf("clib/write: exiting\n" );
    return cnt;
} /* write */

