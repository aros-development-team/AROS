/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>

#include "dos_intern.h"

#define FETCHERR    (-2)

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH4(LONG, FRead,

/*  SYNOPSIS */
        AROS_LHA(BPTR , fh, D1),
        AROS_LHA(APTR , block, D2),
        AROS_LHA(ULONG, blocklen, D3),
        AROS_LHA(ULONG, number, D4),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 54, Dos)

/*  FUNCTION
        Read a number of blocks from a file.
        The read is buffered.

    INPUTS
        fh - Read from this file
        block - The data is put here
        blocklen - This is the size of a single block
        number - The number of blocks

    RESULT
        The number of blocks read from the file or 0 on EOF.
        This function may return fewer than the requested number of blocks.
        IoErr() gives additional information in case of an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Open(), FWrite(), FPutc(), Close()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE  *ptr;
    LONG    res = 0;
    ULONG   fetchsize = number * blocklen;
    ULONG   readsize;

    ptr = block;

    SetIoErr(0);

    if((fetchsize == 0) || (fh == BNULL))
        return 0;

    while(fetchsize > 0)
    {
        res = vbuf_fetch(fh, ptr, fetchsize, DOSBase);
        if (res < 0)
            break;
        ptr += res;
        fetchsize -= res;
    }

    if(res == FETCHERR)
    {
        return EOF;
    }

    readsize = (ULONG)(ptr - (UBYTE *)block);

    return (LONG)(readsize / blocklen);

    AROS_LIBFUNC_EXIT
} /* FRead */

static LONG handle_write_mode(BPTR file, struct DosLibrary * DOSBase)
{
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* If the file is in write mode... */
    if(fh->fh_Flags & FHF_WRITE)
    {
        /* write the buffer (in many pieces if the first one isn't enough). */
        LONG pos = 0;

        while(pos != fh->fh_Pos)
        {
            LONG size = Write(file, BADDR(fh->fh_Buf) + pos, fh->fh_Pos - pos);

            /* An error happened? Return it. */
            if(size < 0)
            {
                return FETCHERR;
            }

            pos += size;
        }

        /* Reinit filehandle. */
        fh->fh_Flags &= ~FHF_WRITE;
        fh->fh_Pos = fh->fh_End = 0;
    }

    return 0;
}

/* Fetches up to remaining buffer content from file buffer
 * Return values:
 *  (-2) on error
 *  (EOF) on EOF
 *  (>0) on successful fetch
 */
LONG vbuf_fetch(BPTR file, UBYTE * buffer, ULONG fetchsize, struct DosLibrary *DOSBase)
{
    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    LONG  size;
    LONG  bufsize;

    if (fh == NULL)
    {
        return FETCHERR;
    }

    if (handle_write_mode(file, DOSBase) == FETCHERR)
    {
        return FETCHERR;
    }

    /* No normal characters left. */
    if(fh->fh_Pos >= fh->fh_End)
    {
        /* Check for a pushed back EOF. */
        if(fh->fh_Pos > fh->fh_End)
        {
            D(bug("FGetC: Weird pos: fh_Pos (%d) > fh_End (%d)\n", fh->fh_Pos, fh->fh_End));
            /* Return EOF. */
            return EOF;
        }

        /* Is there a buffer? */
        if(fh->fh_Buf == BNULL)
        {
            if (NULL == vbuf_alloc(fh, NULL, IOBUFSIZE))
            {
                D(bug("FGetC: Can't allocate buffer\n"));
                return FETCHERR;
            }
        }

        /* Fill the buffer. */
        if (fh->fh_Buf != fh->fh_OrigBuf) {
            D(bug("FGetC: Can't trust fh_BufSize. Using 208 as the buffer size.\n"));
            bufsize = 208;
        } else {
            bufsize = fh->fh_BufSize;
        }
        size = Read(file, BADDR(fh->fh_Buf), bufsize);

        /* Prepare filehandle for data. */
        if(size <= 0)
            size = 0;

        fh->fh_Pos = 0;
        fh->fh_End = size;

        /* No data read? Return EOF. */
        if(size == 0)
        {
            D(bug("FGetC: Tried to Read() to a %d byte buffer, got 0)\n", bufsize));
            return EOF;
        }
    }

    /* If fh_End == 0, simulate an EOF */
    if (fh->fh_End == 0) {
        D(bug("FGetC: Got an EOF via fh_End == 0\n"));
        return EOF;
    }

    /* All OK. Get requested data. */
    size = fh->fh_End - fh->fh_Pos;
    if (size > fetchsize) size = fetchsize;
    if (size == 1) /* Don't do function call for 1 byte reads */
        *buffer = ((UBYTE *)BADDR(fh->fh_Buf))[fh->fh_Pos];
    else
        CopyMem(((UBYTE *)BADDR(fh->fh_Buf)) + fh->fh_Pos, buffer, size);

    fh->fh_Pos += size;

    return size;
}
