/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function dup2().
*/

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libraries/fd.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int dup2(

/*  SYNOPSIS */
        int oldfd,
        int newfd
        )

/*  FUNCTION
        Duplicates a file descriptor.

        The object referenced by the descriptor does not distinguish between
        oldfd and newfd in any way. Thus if newfd and oldfd are duplicate
        references to an open file, read(), write() and lseek() calls all
        move a single pointer into the file, and append mode, non-blocking
        I/O and asynchronous I/O options are shared between the references.
        If a separate pointer into the file is desired, a different object
        reference to the file must be obtained by issuing an additional
        open(2) call.

        The close-on-exec flag on the new file descriptor is unset.

        If oldfd is valid and has the same integer value as newfd, nothing is
        done, and newfd is returned unchanged.

        If newfd is already valid when this function is called, its old
        descriptor is deallocated before this function returns.

        This function fails gracefully if oldfd is invalid.

    INPUTS
        oldfd - The file descriptor to be duplicated
        newfd - The value of the new descriptor we want the old one to be
            duplicated in

    RESULT
        -1 for error or newfd on success

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
        bsdsocket.library/accept(),  open(),  close(),  fcntl(),  pipe()
        bsdsocket.library/socket()

    INTERNALS

******************************************************************************/
{
    fdesc *oldfdesc, *newfdesc;

    /* Fail if old FD is invalid */
    oldfdesc = __getfdesc(oldfd);
    if (!oldfdesc)
    {
        LONG err=0;
        SIPTR result;
        if (oldfd==newfd) {
            /* Validate admission even when no duplication is needed. */
            result=__fdhook(oldfd,__FDH_FCNTL,F_GETFD,0,0,&err);
            if (result<0) {errno=err;return -1;}
            return newfd;
        }
        if (newfd<0) {errno=EBADF;return -1;}
        result=__fdhook(oldfd,__FDH_DUP,newfd,0,0,&err);
        if (result<0) {errno=err;return -1;}
        return newfd;
    }

    /* Do nothing if FDs are identical */
    if (oldfd == newfd)
        return newfd;

    /* Allocate new FD or fail */
    newfdesc = __alloc_fdesc();
    if (!newfdesc)
    {
        errno = ENOMEM;
        return -1;
    }

    /* Initialise new FD */
    newfdesc->fdflags = 0;
    newfdesc->fcb = oldfdesc->fcb;

    /* Put new FD into its slot (and deallocate any FD previously there) */
    int gotfd = __getfdslot(newfd);
    if (gotfd == -1)
    {
        /* Reservation failed: release the descriptor we just allocated so it
           is not leaked. errno was set by __getfdslot(). */
        __free_fdesc(newfdesc);
        return -1;
    }

    /* Bumping the shared fcb's reference count must be atomic against a
       concurrent close() of another descriptor onto the same fcb. */
    __fcb_lock(newfdesc->fcb);
    newfdesc->fcb->opencount++;
    __fcb_unlock(newfdesc->fcb);
    __setfdesc(gotfd, newfdesc);

    return gotfd;
}

