/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Change the position in a stream.
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#define POSIXC_NOSTDIO_DECL

#include "__stdio.h"
#include "__fdesc.h"
#include "__dos64.h"

#include <stdio.h>

/*
 * fseeko engine. Positions are handled in 64 bits throughout; the
 * __dos64 helpers use dos64.library when it is available and fall
 * back to the 32-bit dos.library calls (failing with
 * ERROR_OBJECT_TOO_LARGE for unrepresentable positions) when not.
 */
int __fseeko64 (
        FILE * stream,
        off64_t  offset,
        int    whence)
{
    QUAD cnt = 0;
    QUAD finalseekposition = 0;
    QUAD eofposition = 0;
    BPTR fh = BNULL;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        return -1;
    }

    if (fdesc->fcb->privflags & _FCB_ISDIR)
    {
        errno = EISDIR;
        return -1;
    }

    fh = fdesc->fcb->handle;

    /* This is buffered IO, flush the buffer before any Seek */
    Flush (fh);

    /* Handling for fseeko specific behaviour (not all cases handled) */
    /* Get current position */
    cnt = __dos64_getpos (fdesc->fcb);
    if (cnt == -1)
    {
        errno = __stdc_ioerr2errno (IoErr ());
        return -1;
    }

    /* Get file size */
    eofposition = __dos64_getsize (fdesc->fcb);
    if (eofposition == -1)
    {
        /* Does not happen on sfs/affs */
        errno = EBADF;
        return -1;
    }

    switch(whence)
    {
        case SEEK_SET: finalseekposition = offset; break;
        case SEEK_CUR: finalseekposition = cnt + offset; break;
        case SEEK_END: finalseekposition = eofposition + offset; break;
        default:
            errno = EINVAL;
            return -1;
    }

    /* Check conditions */
    /* Seek before beginning of file */
    if (finalseekposition < 0)
    {
        errno = EINVAL;
        return -1;
    }

    /* Seek beyond end of file and in write mode */
    if (finalseekposition > eofposition)
    {
        if (fdesc->fcb->flags & O_WRITE)
        {
            /* Write '0' to fill up to requested size
             * compatible fseeko does not write but allows write */
            QUAD bytestowrite = finalseekposition - eofposition;
            QUAD chunkcount = bytestowrite / 128;
            LONG remainder = bytestowrite - (chunkcount * 128);
            char zeroarray[128] = {0};

            __dos64_seek (fdesc->fcb, 0, OFFSET_END);
            while (chunkcount > 0)
            {
                LONG n = (chunkcount > 0x100000) ? 0x100000 : (LONG)chunkcount;
                FWrite (fh, zeroarray, 128, n);
                chunkcount -= n;
            }
            if (remainder > 0)
                FWrite (fh, zeroarray, remainder, 1);
            Flush (fh);
        }
    }

    cnt = __dos64_seek (fdesc->fcb, finalseekposition, OFFSET_BEGINNING);

    if (cnt == -1)
    {
        LONG ioerr = IoErr ();
        errno = (ioerr == ERROR_OBJECT_TOO_LARGE)
            ? EOVERFLOW : __stdc_ioerr2errno (ioerr);
        return -1;
    }

    /* It's specified that upon success fseeko should clear EOF flag
       so here we go.
    */
    stream->flags &= ~(__POSIXC_STDIO_EOF);

    return 0;
} /* __fseeko64 */

#if (__WORDSIZE < 64)
/*
 * 32-bit off_t variant: same engine, the offset simply widens.
 */
int __fseeko (
        FILE * stream,
        off_t  offset,
        int    whence)
{
    return __fseeko64 (stream, (off64_t)offset, whence);
} /* __fseeko */
#else
/*
 * on 64bit fseeko is an alias of fseeko64
 */
AROS_MAKE_ALIAS(__fseeko64,__fseeko);
#endif
