/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
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
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    LONG  size;

    if (fh == NULL)
    {
    	return EOF;
    }
    
    /* If the file is in write mode... */
    if(fh->fh_Flags & FHF_WRITE)
    {
        /* write the buffer (in many pieces if the first one isn't enough). */
        UBYTE  *pos = fh->fh_Buf;

        while(pos != fh->fh_Pos)
        {
            size = Write(file, pos, fh->fh_Pos - pos);

            /* An error happened? Return it. */
            if(size < 0)
	    {
                return EOF;
            }

            pos += size;
        }

        /* Reinit filehandle. */
        fh->fh_Flags &= ~FHF_WRITE;
        fh->fh_Pos = fh->fh_End = fh->fh_Buf;
    }

    /* No normal characters left. */
    if(fh->fh_Pos >= fh->fh_End)
    {
        /* Check for a pushed back EOF. */
        if(fh->fh_Pos > fh->fh_End)
        {
            /* Reinit filehandle and return EOF. */
            fh->fh_Pos = fh->fh_End;
	    SetIoErr(0);

            return EOF;
        }

        /* Is there a buffer? */
        if(fh->fh_Buf == NULL)
        {
            if (NULL == vbuf_alloc(fh, IOBUFSIZE))
            {
                return(EOF);
            }
        }

        /* Fill the buffer. */
        size = Read(file, fh->fh_Buf, fh->fh_Size);

        /* Prepare filehandle for data. */
        if(size <= 0)
            size = 0;

        fh->fh_Pos = fh->fh_Buf;
        fh->fh_End = fh->fh_Buf + size;

        /* No data read? Return EOF. */
        if(size == 0)
	{
            return EOF;
	}
    }

    /* All OK. Get data. */
    return *fh->fh_Pos++;

    AROS_LIBFUNC_EXIT
} /* FGetC */
