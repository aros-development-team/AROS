/*
    (C) 1995-97 AROS - The Amiga Research OS
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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* If the file is in write mode flush it */
    if(fh->fh_Flags & FHF_WRITE)
	Flush(file);
    else
    {
	/* Read mode. Just reinit the buffers. We can't call
	   Flush() in this case as that would end up in recursion. */
	fh->fh_Pos = fh->fh_End = fh->fh_Buf;
    }

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_SEEK, DOSBase);

    iofs.IOFS.io_Device  = fh->fh_Device;
    iofs.IOFS.io_Unit    = fh->fh_Unit;

    iofs.io_Union.io_SEEK.io_Offset   = (QUAD)position;
    iofs.io_Union.io_SEEK.io_SeekMode = mode;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError)
	return -1;
    else
	return (LONG)iofs.io_Union.io_SEEK.io_Offset;

    AROS_LIBFUNC_EXIT
} /* Seek */
