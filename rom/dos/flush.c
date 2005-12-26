/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "dos_intern.h"

#define DEBUG 1
#include <aros/debug.h>

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle. */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* The file must be in write mode. */
    if( fh->fh_Flags & FHF_WRITE )
    {
	/* Handle append mode. */
	if( fh->fh_Flags & FHF_APPEND )
	{
	    InternalSeek( fh, 0, OFFSET_END, DOSBase );
	}
	
	return InternalFlush( fh, DOSBase );
    }
    else if( fh->fh_Pos < fh->fh_End )
    {
    	int offset = fh->fh_Pos - fh->fh_End;
	
	fh->fh_Pos = fh->fh_End = fh->fh_Buf;
	
        /* Read mode. Try to seek back to the current position. */
        if( InternalSeek( file, offset, OFFSET_CURRENT, DOSBase ) < 0 )
        {
            return FALSE;
        }
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* Flush */
