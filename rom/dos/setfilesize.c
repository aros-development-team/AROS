/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the size of a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, SetFileSize,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,   D1),
	AROS_LHA(LONG, offset, D2),
	AROS_LHA(LONG, mode,   D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 76, Dos)

/*  FUNCTION
	Change the size of a file.

    INPUTS
	file   - filehandle
	offset - relative size
	mode   - OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END

    RESULT
	New size of the file or -1 in case of an error.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_SET_FILE_SIZE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_SET_FILE_SIZE.io_Offset   = (QUAD)offset;
    iofs.io_Union.io_SET_FILE_SIZE.io_SeekMode = mode;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);
    
    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError != 0)
        return -1;    
    else
        return (LONG)iofs.io_Union.io_SET_FILE_SIZE.io_Offset;
    
    AROS_LIBFUNC_EXIT
} /* SetFileSize */
