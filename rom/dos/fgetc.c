/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/11/25 09:50:22  SDuvan
    Updated layout

    Revision 1.7  1998/10/20 16:44:35  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:19  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:27  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:27  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:50  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    LONG  size;

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
                return EOF;

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
            /* No. Get one. */
            fh->fh_Buf = AllocMem(IOBUFSIZE, MEMF_ANY);

            if(fh->fh_Buf == NULL)
            {
                /* Couldn't get buffer. Return error. */
                SetIoErr(ERROR_NO_FREE_STORE);

                return EOF;
            }

            /* Got it. Use it. */
            fh->fh_Flags |= FHF_BUF;
            fh->fh_Size = IOBUFSIZE;
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
            return EOF;
    }
    
    /* All OK. Get data. */
    return *fh->fh_Pos++;

    AROS_LIBFUNC_EXIT
} /* FGetC */
