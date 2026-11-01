/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/exec.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

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
    fcb *rfcb = NULL, *wfcb = NULL;
    fdesc *rdesc = NULL, *wdesc = NULL;
    int rfd, wfd, error;
    /* PIPE:cpipe-%08x-%d, where %x is the getpid(), %d is the nth pipe */
    char pipe_name[5 + 6 + 8 + 1 + 16 + 1];
    static int pipeno = 0;

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
    FreeVec(rfcb);
        if(rdesc)
            __free_fdesc(rdesc);
    FreeVec(wfcb);
        if(wdesc)
            __free_fdesc(wdesc);
        errno = ENOMEM;
        return -1;
    }

    /* Get the next pipe number */
    Forbid();
    pipeno++;
    Permit();

    snprintf(pipe_name, sizeof(pipe_name), "PIPE:cpipe-%08lx-%d",
               (unsigned long)getpid(), pipeno);
    pipe_name[sizeof(pipe_name)-1] = 0;

    writer = Open(pipe_name, MODE_NEWFILE);
    if (writer)
    {
        reader = Open(pipe_name, MODE_OLDFILE);
        if (!reader)
        {
            DeleteFile(pipe_name);
            Close(writer);
            writer = BNULL;
        }
    }

    if (!writer)
    {
        errno = __stdc_ioerr2errno(IoErr());
        __free_fdesc(rdesc);
        __free_fdesc(wdesc);
        return -1;
    }

    InitSemaphore(&rfcb->io_lock);
    InitSemaphore(&rfcb->ref_lock);
    InitSemaphore(&wfcb->io_lock);
    InitSemaphore(&wfcb->ref_lock);

    rdesc->fdflags = 0;
    rdesc->fcb = rfcb;
    rdesc->fcb->handle    = reader;
    rdesc->fcb->flags     = O_RDONLY;
    rdesc->fcb->opencount = 1;
    rfd = __publish_newfd(rdesc, 0);
    if (rfd < 0)
        goto err;

    wdesc->fdflags = 0;
    wdesc->fcb = wfcb;
    wdesc->fcb->handle    = writer;
    wdesc->fcb->flags     = O_WRONLY;
    wdesc->fcb->opencount = 1;
    wfd = __publish_newfd(wdesc, 0);
    if (wfd < 0)
        goto err;

    pipedes[0] = rfd;
    pipedes[1] = wfd;

    return 0;

err:
    error = errno;
    if (rfd >= 0)
    {
        /* The published reader owns its handle, fdesc and fcb. */
        close(rfd);
    }
    else
    {
        Close(reader);
        __free_fdesc(rdesc);
        FreeVec(rfcb);
    }
    Close(writer);
    __free_fdesc(wdesc);
    FreeVec(wfcb);
    errno = error;
    return -1;
}

