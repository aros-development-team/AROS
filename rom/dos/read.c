/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a couple of bytes from a file.
    Lang: english
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, Read,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,   D1),
	AROS_LHA(APTR, buffer, D2),
	AROS_LHA(LONG, length, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 7, Dos)

/*  FUNCTION
	Read some data from a given file. The request is directly
	given to the filesystem - no buffering is involved. For
	small amounts of data it's probably better to use the
	buffered I/O routines.

    INPUTS
	file   - filehandle
	buffer - pointer to buffer for the data
	length - number of bytes to read. The filesystem is
		 advised to try to fulfill the request as good
		 as possible.

    RESULT
	The number of bytes actually read, 0 if the end of the
	file was reached, -1 if an error happened. IoErr() will
	give additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    ASSERT_VALID_PTR(fh);
    ASSERT_VALID_PTR(fh->fh_Device);
    ASSERT_VALID_PTR(fh->fh_Unit);
    ASSERT_VALID_PTR(buffer);

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_READ, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_READ.io_Buffer = buffer;
    iofs.io_Union.io_READ.io_Length = length;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    /* Set error code and return */

    if(iofs.io_DosError != 0)
    {
        SetIoErr(iofs.io_DosError);
	return -1;
    }
    else
	return iofs.io_Union.io_READ.io_Length;

    AROS_LIBFUNC_EXIT
} /* Read */
