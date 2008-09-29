/*
    Copyright © 2004-2007, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function fsync().
*/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__open.h"

/*****************************************************************************

    NAME */

#include <fcntl.h>
#include <unistd.h>

	int fsync(

/*  SYNOPSIS */
	int fd)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc || !(fdesc->flags & O_WRITE))
    {
        errno = EBADF;
        return -1;
    }

    if (!Flush((BPTR) fdesc->fh))
    {
        errno = IoErr2errno(IoErr());
        return -1;
    }
    
    return 0;
} /* fsync */

