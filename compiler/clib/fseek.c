/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Change the position in a stream.
*/

#include <fcntl.h>
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fseek (

/*  SYNOPSIS */
	FILE * stream,
	long   offset,
	int    whence)

/*  FUNCTION
	Change the current position in a stream.

    INPUTS
	stream - Modify this stream
	offset, whence - How to modify the current position. whence
		can be SEEK_SET, then offset is the absolute position
		in the file (0 is the first byte), SEEK_CUR then the
		position will change by offset (ie. -5 means to move
		5 bytes to the beginning of the file) or SEEK_END.
		SEEK_END means that the offset is relative to the
		end of the file (-1 is the last byte and 0 is
		the EOF).

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES

    EXAMPLE

    BUGS
    Not fully compatible with iso fseek, especially in 'ab' and 'a+b'
    modes

    SEE ALSO
	fopen(), fwrite()

    INTERNALS

******************************************************************************/
{
    int cnt = 0;
    int finalseekposition = 0;
    int eofposition = 0;
    struct FileInfoBlock *fib = NULL;
    BPTR fh = BNULL;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	    errno = EBADF;
	    return -1;
    }

    fh = (BPTR)(fdesc->fcb->fh);

    /* This is buffered IO, flush the buffer before any Seek */
    Flush (fh);

    /* Handling for fseek specific behaviour (not all cases handled) */
    /* Get current position */
    cnt = Seek (fh, 0, OFFSET_CURRENT);
    if (cnt == -1)
    {
    	errno = __arosc_ioerr2errno (IoErr ());
        return -1;
    }

    /* Get file size */
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        errno = __arosc_ioerr2errno(IoErr());
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
            /* Write '0' to fill up to requested size - compatible fseek does not write but allows write */
            int i = 0;
            int bytestowrite = finalseekposition - eofposition;
            int chunkcount = (bytestowrite)/128;
            char zeroarray[128] = {0};

            Seek (fh, 0, OFFSET_END);
            for (i = 0; i < chunkcount; i++)
                FWrite(fh, (STRPTR)zeroarray, 128, 1);
            FWrite(fh, (STRPTR)zeroarray, bytestowrite - (chunkcount * 128), 1);
            Flush (fh);
        }
    }

    cnt = Seek (fh, finalseekposition, OFFSET_BEGINNING);

    if (cnt == -1)
    	errno = __arosc_ioerr2errno (IoErr ());
    else
    {
        /* It's specified that upon success fseek should clear EOF flag
           so here we go.
        */
        stream->flags &= ~(_STDIO_EOF);
    	cnt = 0;
    }

    return cnt;
} /* fseek */
