/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

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

    /* Write data */
    *fh->fh_Pos++ = character;

   /* Check if there is still some space in the buffer */
    if (fh->fh_Pos >= fh->fh_End || (fh->fh_Flags & FHF_LINEBUF && character=='\n') || fh->fh_Flags & FHF_NOBUF)
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

    return character;

    AROS_LIBFUNC_EXIT
} /* FPutC */
