/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <dos/stdio.h>
#include <dos/dosextens.h>

#include "dos_intern.h"


/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, FGetC,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 51, Dos)

/*  FUNCTION
        Get a character from a buffered file. Buffered I/O is more efficient
        for small amounts of data but less for big chunks. You have to
        use Flush() between buffered and non-buffered I/O or you'll
        clutter your I/O stream.

    INPUTS
        file   - filehandle

    RESULT
        The character read or EOF if the file ended or an error happened.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IoErr(), Flush()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    LONG  size;
    LONG  bufsize;

    if (fh == NULL)
    {
        return EOF;
    }
    
    /* If the file is in write mode... */
    if(fh->fh_Flags & FHF_WRITE)
    {
        /* write the buffer (in many pieces if the first one isn't enough). */
        LONG pos = 0;

        while(pos != fh->fh_Pos)
        {
            size = Write(file, BADDR(fh->fh_Buf) + pos, fh->fh_Pos - pos);

            /* An error happened? Return it. */
            if(size < 0)
            {
                return EOF;
            }

            pos += size;
        }

        /* Reinit filehandle. */
        fh->fh_Flags &= ~FHF_WRITE;
        fh->fh_Pos = fh->fh_End = 0;
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
                return(EOF);
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

    /* All OK. Get data. */
    return ((UBYTE *)BADDR(fh->fh_Buf))[fh->fh_Pos++];

    AROS_LIBFUNC_EXIT
} /* FGetC */
