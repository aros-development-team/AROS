/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function ftruncate().
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <errno.h>
#include "__fdesc.h"
#include "__dos64.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int ftruncate (

/*  SYNOPSIS */
        int   fd,
        off_t length)

/*  FUNCTION
        Truncate a file to a specified length

    INPUTS
        fd     - the descriptor of the file being truncated.
                 The file must be open for writing
        lenght - The file will have at most this size

    RESULT
        0 on success or -1 on errorr.

    NOTES
        If the file previously was larger than this size, the extra  data
        is  lost.   If  the  file  previously  was  shorter, it is
        unspecified whether the  file  is  left  unchanged  or  is
        extended.  In  the  latter case the extended part reads as
        zero bytes.


    EXAMPLE

    BUGS

    SEE ALSO
        open(), truncate()

    INTERNALS

******************************************************************************/
{
    QUAD oldpos;
    QUAD size;

    fdesc *fdesc = __getfdesc(fd);

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

    if (!(fdesc->fcb->flags & (O_WRONLY|O_RDWR)))
    {
        errno = EINVAL;
        return -1;
    }

    oldpos = __dos64_getpos(fdesc->fcb);
    size   = __dos64_getsize(fdesc->fcb);

    if (__dos64_setfilesize(fdesc->fcb, length, OFFSET_BEGINNING) == -1)
    {
        LONG ioerr = IoErr();
        errno = (ioerr == ERROR_OBJECT_TOO_LARGE)
            ? EFBIG : __stdc_ioerr2errno(ioerr);
        return -1;
    }
    else
    if (size != -1 && size < length)
    {
        char buf[16]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        QUAD left = length - size;

        __dos64_seek(fdesc->fcb, size, OFFSET_BEGINNING);

        while (left >= 16)
        {
            FWrite(fdesc->fcb->handle, buf, 16, 1);
            left -= 16;
        }
        if (left)
            FWrite(fdesc->fcb->handle, buf, left, 1);

        Flush(fdesc->fcb->handle);
    }

    /* Restore the original position */
    if (oldpos != -1)
        __dos64_seek(fdesc->fcb, oldpos, OFFSET_BEGINNING);

    return 0;
}
