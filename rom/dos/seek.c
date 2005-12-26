/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the current read/write position in a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, Seek,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,     D1),
	AROS_LHA(LONG, position, D2),
	AROS_LHA(LONG, mode,     D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 11, Dos)

/*  FUNCTION
	Changes the current read/write position in a file and/or
	reads the current position, e.g to get the current position
	do a Seek(file,0,OFFSET_CURRENT).

	This function may fail (obviously) on certain devices such
	as pipes or console handlers.

    INPUTS
	file	 - filehandle
	position - relative offset in bytes (positive, negative or 0).
	mode	 - Where to count from. Either OFFSET_BEGINNING,
		   OFFSET_CURRENT or OFFSET_END.

    RESULT
	Absolute position in bytes before the Seek(), -1 if an error
	happened. IoErr() will give additional information in that case.

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
    LONG               offset = 0, ret;

    /* If the file is in append mode, seeking is not allowed. */
    if( fh->fh_Flags & FHF_APPEND )
    {
	return InternalSeek( fh, 0, OFFSET_CURRENT, DOSBase );
    }

    /* If the file is in write mode flush it. */
    if( fh->fh_Flags & FHF_WRITE )
    {
	InternalFlush( fh, DOSBase );
    }
    else
    {
	/* Read mode. Adjust the offset so that buffering is
	   taken into account. */
	if (fh->fh_Pos < fh->fh_End && mode == OFFSET_CURRENT)
	    offset = (LONG)(fh->fh_Pos - fh->fh_End);
	    
	    
        /* Read mode. Just reinit the buffers. We can't call
           Flush() in this case as that would end up in
	   recursion. */
        fh->fh_Pos = fh->fh_End = fh->fh_Buf;
    }

    ret = InternalSeek( fh, position + offset, mode, DOSBase );
    return (ret == -1) ? -1 : (ret + offset);
    
    AROS_LIBFUNC_EXIT
} /* Seek */
