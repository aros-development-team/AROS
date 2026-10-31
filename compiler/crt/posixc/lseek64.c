/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Reposition read/write file offset.
*/
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include "__fdesc.h"
#include "__dos64.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        __off64_t lseek64 (

/*  SYNOPSIS */
        int    filedes,
        __off64_t  offset,
        int    whence)

/*  FUNCTION
        Reposition read/write file offset

    INPUTS
        filedef - the filedescriptor being modified
        offset, whence -
                  How to modify the current position. whence
                  can be SEEK_SET, then offset is the absolute position
                  in the file (0 is the first byte), SEEK_CUR then the
                  position will change by offset (ie. -5 means to move
                  5 bytes to the beginning of the file) or SEEK_END.
                  SEEK_END means that the offset is relative to the
                  end of the file (-1 is the last byte and 0 is
                  the EOF).

    RESULT
        The new position on success and -1 on error. If an error occurred, the global
        variable errno is set.

    NOTES

    EXAMPLE

    BUGS
        Since it's not possible to use Seek() for directories, this
        implementation fails with EISDIR for directory file descriptors.

    SEE ALSO
        __posixc_fopen(), __posixc_fwrite()

    INTERNALS

******************************************************************************/
{
    QUAD cnt;
    fdesc *fdesc = __getfdesc(filedes);

    if (!fdesc)
    {
        errno = EBADF;
        return -1;
    }

    if(fdesc->fcb->privflags & _FCB_ISDIR)
    {
        errno = EISDIR;
        return -1;
    }

    FLUSHONREADCHECK

    switch (whence)
    {
        case SEEK_SET: whence = OFFSET_BEGINNING; break;
        case SEEK_CUR: whence = OFFSET_CURRENT; break;
        case SEEK_END: whence = OFFSET_END; break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* POSIX: a position beyond EOF is accepted and does not change the
       file; the gap is only filled when something is written there. */
    cnt = __dos64_lseek (fdesc->fcb, offset, whence);
    if (cnt == -1)
        goto error;
    if (cnt != (QUAD)(__off64_t)cnt)
    {
        errno = EOVERFLOW;
        return (__off64_t) -1;
    }

    return (__off64_t) cnt;
error:
    {
        LONG ioerr = IoErr ();
        errno = (ioerr == ERROR_OBJECT_TOO_LARGE)
            ? EOVERFLOW : __stdc_ioerr2errno (ioerr);
    }
    return (__off64_t) -1;
} /* lseek64 */
