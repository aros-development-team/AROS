/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/11/18 12:17:31  SDuvan
    Updated layout

    Revision 1.7  1998/10/20 16:44:37  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:20  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:28  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:29  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:46  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:51  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, Flush,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 60, Dos)

/*  FUNCTION
	Flushes any pending writes on the file. If the file was used
	for input and there is still some data to read it tries to
	seek back to the expected position.

    INPUTS
	file - filehandle

    RESULT
	!= 0 on success, 0 on error. IoErr() gives additional information
	in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

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
    long success = 1;

    /* If the file is in write mode... */
    if(fh->fh_Flags & FHF_WRITE)
    {
	UBYTE *pos;

	/* Unset write flag */
	fh->fh_Flags &= ~FHF_WRITE;

	/* Write the data. (In many pieces if the first one isn't enough). */
	pos = fh->fh_Buf;
        while(pos != fh->fh_Pos)
        {
            LONG size;

	    size = Write(file,pos,fh->fh_Pos-pos);

	    /* An error happened? No success. */
	    if(size < 0)
	    {
	        success = 0;
		break;
	    }

	    pos += size;
	}
    }
    else if(fh->fh_Pos < fh->fh_End)
    {
        /* Read mode. Try to seek back to the current position. */
        if(Seek(file, fh->fh_Pos - fh->fh_End, OFFSET_CURRENT) < 0)
            success = 0;
    }

    /* Reinit the buffer and return. */
    fh->fh_Pos = fh->fh_End = fh->fh_Buf;

    return success;

    AROS_LIBFUNC_EXIT
} /* Flush */
