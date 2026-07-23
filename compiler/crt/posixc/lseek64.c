/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Reposition read/write file offset.
*/
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <clib/macros.h>
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
        File is extended with zeros if desired position is beyond the end of
        file.

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

    cnt = __dos64_seek (fdesc->fcb, offset, whence);

    if (cnt == -1)
    {
        if(IoErr() == ERROR_SEEK_ERROR)
        {
            LONG saved_error = IoErr();
            /* Most likely we tried to seek behind EOF. POSIX lseek allows
               that, and if anything is written at the end on the gap,
               reads from the gap should return 0 unless some real data
               is written there. Since implementing it would be rather
               difficult, we simply extend the file by writing zeros
               and hope for the best. */
            QUAD abs_cur_pos = __dos64_getpos(fdesc->fcb);
            if(abs_cur_pos == -1)
                goto error;
            QUAD file_size = __dos64_getsize(fdesc->fcb);
            if(file_size == -1)
                goto error;
            /* Now compute how much we have to extend the file */
            QUAD abs_new_pos = 0;
            switch(whence)
            {
                case OFFSET_BEGINNING: abs_new_pos = offset; break;
                case OFFSET_CURRENT: abs_new_pos = abs_cur_pos + offset; break;
                case OFFSET_END: abs_new_pos = file_size + offset; break;
            }
            if(abs_new_pos > file_size)
            {
                ULONG bufsize = 4096;
                APTR zeros = AllocMem(bufsize, MEMF_ANY | MEMF_CLEAR);
                if(!zeros)
                {
                    /* Restore previous position */
                    __dos64_seek(fdesc->fcb, abs_cur_pos, OFFSET_BEGINNING);
                    errno = ENOMEM;
                    return -1;
                }

                if(__dos64_seek(fdesc->fcb, 0, OFFSET_END) == -1)
                {
                    FreeMem(zeros, bufsize);
                    goto error;
                }
                QUAD towrite = abs_new_pos - file_size;
                do
                {
                    Write(fdesc->fcb->handle, zeros, MIN(towrite, bufsize));
                    towrite -= bufsize;
                }
                while(towrite > 0);

                FreeMem(zeros, bufsize);
            }
            else
            {
                /* Hmm, that's strange. Looks like ERROR_SEEK_ERROR has
                   been caused by something else */
                SetIoErr(saved_error);
                goto error;
            }
        }
        else
            goto error;
    }

    cnt = __dos64_getpos(fdesc->fcb);
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
