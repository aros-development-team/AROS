/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function close()
    Lang: english
*/

#include <unistd.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include "__open.h"
#include "__errno.h"

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
    GETUSER;

    fdesc *fdesc;

    if (!(fdesc = __getfdesc(fd)))
    {
        errno = EBADF;

	return -1;
    }

    if (--fdesc->opencount == 0)
    {
	if (
	    fdesc->fh!=__stdfiles[STDIN_FILENO] &&
	    fdesc->fh!=__stdfiles[STDOUT_FILENO] &&
	    fdesc->fh!=__stdfiles[STDERR_FILENO] &&
	    !Close(fdesc->fh)
	)
	{
	    fdesc->opencount++;
	    errno = IoErr2errno(IoErr());

	    return -1;
        }

	free(fdesc);
    }

    __setfdesc(fd, NULL);

    return 0;
} /* close */

