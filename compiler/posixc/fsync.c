/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function fsync().
*/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include <fcntl.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
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

    if (!fdesc || !(fdesc->fcb->flags & O_WRITE))
    {
        errno = EBADF;
        return -1;
    }

    if (!Flush((BPTR) fdesc->fcb->fh))
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }
    
    return 0;
} /* fsync */

