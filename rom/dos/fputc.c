/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2001/01/06 15:24:00  SDuvan
    Cleaned up, use SetioErr() instead of custom procedure

    Revision 1.8  1998/10/20 16:44:37  hkiel
    Amiga Research OS

    Revision 1.7  1997/01/27 00:36:20  ldp
    Polish

    Revision 1.6  1996/12/09 13:53:28  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.5  1996/10/24 15:50:29  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.3  1996/08/12 14:22:57  digulla
    Removed irritating empty line

    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, FPutC,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,      D1),
	AROS_LHA(LONG, character, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 52, Dos)

/*  FUNCTION

    INPUTS
	file	  - Filehandle to write to.
	character - Character to write.

    RESULT
	The character written or EOF in case of an error.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FGetC(), IoErr()

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

    /* Check if file is in write mode */
    if (!(fh->fh_Flags & FHF_WRITE))
    {
	if (fh->fh_Pos < fh->fh_End)
	{
	    /* Read mode. Try to seek back to the current position. */
	    if (Seek(file, fh->fh_Pos - fh->fh_End, OFFSET_CURRENT) < 0)
	    {
		fh->fh_Pos = fh->fh_End = fh->fh_Buf;

		return EOF;
	    }
	}
	
	/* Is there a buffer? */
	if (fh->fh_Buf == NULL)
	{
	    /* No. Get one. */
	    fh->fh_Buf = AllocMem(IOBUFSIZE, MEMF_ANY);

	    if (fh->fh_Buf == NULL)
	    {
		/* Couldn't get buffer. Return error. */
		SetIoErr(ERROR_NO_FREE_STORE);

		return EOF;
	    }
	    
	    /* Got it. Use it. */
	    fh->fh_Flags |= FHF_BUF;
	    fh->fh_Size = IOBUFSIZE;
	}
	
	/* Prepare buffer */
	fh->fh_Pos = fh->fh_Buf;
	fh->fh_End = fh->fh_Buf + fh->fh_Size;
	fh->fh_Flags |= FHF_WRITE;
    }
    
    /* Check if there is still some space in the buffer */
    if (fh->fh_Pos >= fh->fh_End)
    {
	UBYTE *pos;
	
	/* Write the data. (In many pieces if the first one isn't enough). */
	pos = fh->fh_Buf;

	while (pos != fh->fh_Pos)
	{
	    LONG size;

	    size = Write(file, pos, fh->fh_Pos - pos);

	    /* An error happened? No success. */
	    if (size < 0)
	    {
		fh->fh_Pos = fh->fh_End = fh->fh_Buf;
		fh->fh_Flags &= ~FHF_WRITE;

		return EOF;
	    }

	    pos += size;
	}

	/* Reset buffer */
	fh->fh_Pos = fh->fh_Buf;
    }

    /* Write data and return */
    *fh->fh_Pos++ = character;

    return character;

    AROS_LIBFUNC_EXIT
} /* FPutC */
