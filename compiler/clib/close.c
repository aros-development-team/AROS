/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function close()
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int close (

/*  SYNOPSIS */
	int fd)

/*  FUNCTION
	Closes an open file. If this is the last file descriptor
	associated with this file, then all allocated resources
	are freed, too.

    INPUTS
	fd - The result of a successful open()

    RESULT
	-1 for error or zero on success.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	open(), read(), write(), fopen()

    INTERNALS

    HISTORY
	15.01.97 digulla created

******************************************************************************/
{
    FILENODE * fn;

    ForeachNode (&__stdio_files,fn)
    {
	if (fn->fd == fd)
	{
	    Remove ((struct Node *)fn);
	    Close ((BPTR)fn->File.fh);
	    FreeMem (fn, sizeof (FILENODE));

	    return 0;
	}
    }

    errno = EBADF;
    return -1;
} /* close */

