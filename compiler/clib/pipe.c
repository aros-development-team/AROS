/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/exec.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "__errno.h"
#include "__open.h"

/*****************************************************************************

    NAME */

	int pipe(

/*  SYNOPSIS */
	int *pipedes)

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
    BPTR reader, writer;
    fcb *rfcb, *wfcb;
    fdesc *rdesc, *wdesc;

    if (!pipedes)
    {
	errno = EFAULT;

	return -1;
    }

    if (
	(rfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) == NULL ||
	(rdesc = __alloc_fdesc()) == NULL ||
	(wfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) == NULL ||
	(wdesc = __alloc_fdesc()) == NULL
    )
    {
	if(rfcb)
	    FreeVec(rfcb);
	if(rdesc)
	    __free_fdesc(rdesc);
	if(wfcb)
	    FreeVec(wfcb);
	if(wdesc)
	    __free_fdesc(wdesc);
	errno = ENOMEM;
	return -1;
    }

    if (Pipe("XPIPE:", &reader, &writer) != DOSTRUE) {
        errno = IoErr2errno(IoErr());
        __free_fdesc(rdesc);
        __free_fdesc(wdesc);
        return -1;
    }

    pipedes[0] = __getfdslot(__getfirstfd(0));
    rdesc->fdflags = 0;
    rdesc->fcb = rfcb;
    rdesc->fcb->fh        = reader;
    rdesc->fcb->flags     = O_RDONLY;
    rdesc->fcb->opencount = 1;
    __setfdesc(pipedes[0], rdesc);

    pipedes[1] = __getfdslot(__getfirstfd(pipedes[0]));
    wdesc->fdflags = 0;
    wdesc->fcb = wfcb;
    wdesc->fcb->fh        = writer;
    wdesc->fcb->flags     = O_WRONLY;
    wdesc->fcb->opencount = 1;
    __setfdesc(pipedes[1], wdesc);

    return 0;
}

