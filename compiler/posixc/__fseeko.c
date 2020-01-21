/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

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

#include <stdio.h>

#if (__WORDSIZE < 64)

/*
 * 32bit version of fseeko. only handles <=2GB files.
 */

int __fseeko (
	FILE * stream,
	off_t  offset,
	int    whence)
{
    off_t cnt = 0;
    off_t finalseekposition = 0;
    off_t eofposition = 0;
    struct FileInfoBlock *fib = NULL;
    BPTR fh = BNULL;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	    errno = EBADF;
	    return -1;
    }

    fh = fdesc->fcb->handle;

    /* This is buffered IO, flush the buffer before any Seek */
    Flush (fh);

    /* Handling for fseeko specific behaviour (not all cases handled) */
    /* Get current position */
    cnt = Seek (fh, 0, OFFSET_CURRENT);
    if (cnt == -1)
    {
        errno = __stdc_ioerr2errno (IoErr ());
        return -1;
    }

    /* Get file size */
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    if (ExamineFH(fh, fib))
        eofposition = fib->fib_Size;
    else
    {
        /* Does not happen on sfs/affs */
        FreeDosObject(DOS_FIB, fib);
        fib = NULL;
        errno = EBADF;
        return -1;
    }

    FreeDosObject(DOS_FIB, fib);
    fib = NULL;

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
            int bytestowrite = finalseekposition - eofposition;
            int chunkcount = (bytestowrite)/128;
            int remainder = bytestowrite - (chunkcount * 128);
            char zeroarray[128] = {0};

            Seek (fh, 0, OFFSET_END);
            if (chunkcount > 0)
                FWrite (fh, zeroarray, 128, chunkcount);
            if (remainder > 0)
                FWrite (fh, zeroarray, remainder, 1);
            Flush (fh);
        }
    }

    cnt = Seek (fh, finalseekposition, OFFSET_BEGINNING);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());
    else
    {
        /* It's specified that upon success fseeko should clear EOF flag
           so here we go.
        */
        stream->flags &= ~(__POSIXC_STDIO_EOF);
    	cnt = 0;
    }

    return cnt;
} /* __fseeko */
#endif

/*
 * 64bit version of fseeko.  Must be able to hanlde both 32bit and 64bit filesystems !
 */

int __fseeko64 (
	FILE * stream,
	off64_t  offset,
	int    whence)
{
    off_t cnt = 0;
    off_t finalseekposition = 0;
    off_t eofposition = 0;
    struct FileInfoBlock *fib = NULL;
    BPTR fh = BNULL;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	    errno = EBADF;
	    return -1;
    }

    if (((offset <= LONG_MIN) || (offset >= LONG_MAX)) && !(fdesc->fcb->privflags & _FCB_FS64))
    {
        /* asked to seek too far on a 32bit filesystem.. */
        errno = EBADF;
        return -1;
    }

    fh = fdesc->fcb->handle;

    /* This is buffered IO, flush the buffer before any Seek */
    Flush (fh);

    /* Handling for fseeko specific behaviour (not all cases handled) */
    /* Get current position */
    cnt = Seek (fh, 0, OFFSET_CURRENT);
    if (cnt == -1)
    {
        errno = __stdc_ioerr2errno (IoErr ());
        return -1;
    }

    /* Get file size */
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }

    if (ExamineFH(fh, fib))
        eofposition = fib->fib_Size;
    else
    {
        /* Does not happen on sfs/affs */
        FreeDosObject(DOS_FIB, fib);
        fib = NULL;
        errno = EBADF;
        return -1;
    }

    FreeDosObject(DOS_FIB, fib);
    fib = NULL;

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
            int bytestowrite = finalseekposition - eofposition;
            int chunkcount = (bytestowrite)/128;
            int remainder = bytestowrite - (chunkcount * 128);
            char zeroarray[128] = {0};

            Seek (fh, 0, OFFSET_END);
            if (chunkcount > 0)
                FWrite (fh, zeroarray, 128, chunkcount);
            if (remainder > 0)
                FWrite (fh, zeroarray, remainder, 1);
            Flush (fh);
        }
    }

    cnt = Seek (fh, finalseekposition, OFFSET_BEGINNING);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());
    else
    {
        /* It's specified that upon success fseeko should clear EOF flag
           so here we go.
        */
        stream->flags &= ~(__POSIXC_STDIO_EOF);
    	cnt = 0;
    }

    return cnt;
} /* __fseeko64 */

#if (__WORDSIZE==64)
/*
 * on 64bit fseeko is an alias of fseeko64
 */
AROS_MAKE_ALIAS(__fseeko64,__fseeko);
#endif
