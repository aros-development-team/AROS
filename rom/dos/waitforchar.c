/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Waits for a character to arrive at a filehandle.
    Lang: English
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <exec/types.h>

	AROS_LH2(BOOL, WaitForChar,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),
	AROS_LHA(LONG, timeout, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 34, Dos)

/*  FUNCTION
	Wait for a character to arrive at a filehandle. The filehandle
	can be either a console handle, or a regular file. For a regular
	file most filesystems will return a character immediately, but
	sometimes (for example a network handler) the character may not
	have arrived.

    INPUTS
	file		- File to wait for a character on.
	timeout		- Number of microseconds to wait for the character
			  to arrive. A value of 0 says to wait indefinately.
    RESULT
	!= 0	if a character arrived before the timeout expired
	== 0	if no character arrived

    NOTES
	Many filesystems do not implement this function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* NULL-pointers are okay. */
    if(file == NULL)
        return 0;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_WAIT_CHAR, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_WAIT_CHAR.io_Timeout = timeout;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError == 0)
        return iofs.io_Union.io_WAIT_CHAR.io_Success;
    else
        return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* WaitForChar */
