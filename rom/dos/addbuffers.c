/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Add or remove cache memory from a filesystem.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, AddBuffers,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, devicename, D1),
	AROS_LHA(LONG,         numbuffers, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 122, Dos)

/*  FUNCTION
	Add or remove cache memory from a filesystem.

    INPUTS
	devicename  --  NUL terminated dos device name.
	numbuffers  --  Number of buffers to add. May be negative.

    RESULT
	!= 0 on success (IoErr() gives the actual number of buffers),
	0 else (IoErr() gives the error code).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    The error value in case of a filesystem error will be reported in
    the io_MORE_CACHE.io_NumBuffers field.

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    BOOL success = FALSE;

    /* Use stackspace for IO request. */
    struct IOFileSys iofs;
    
    InitIOFS(&iofs, FSA_MORE_CACHE, DOSBase);

    /* Now the specific intialization */

    /* Get the device corresponding to 'devicename' */
    iofs.IOFS.io_Device = GetDevice(devicename, &iofs.IOFS.io_Unit, DOSBase);

    if(iofs.IOFS.io_Device == NULL)
	return FALSE;

    iofs.io_Union.io_MORE_CACHE.io_NumBuffers = numbuffers;
    
    /* Send the request. */
    DosDoIO(&iofs.IOFS);
    
    /* Set error code */
    if(iofs.io_DosError == 0)
    {
	/* IoErr() gives the number of buffers! */
	SetIoErr(iofs.io_Union.io_MORE_CACHE.io_NumBuffers);
	success = TRUE;
    }
    else
	SetIoErr(iofs.io_DosError);

    return success;

    AROS_LIBFUNC_EXIT
} /* AddBuffers */
