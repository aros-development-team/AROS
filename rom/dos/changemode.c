/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Change the mode of a filehandle or -lock.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, ChangeMode,

/*  SYNOPSIS */
	AROS_LHA(ULONG, type,    D1),
	AROS_LHA(BPTR,  object,  D2),
	AROS_LHA(ULONG, newmode, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 75, Dos)

/*  FUNCTION
	Try to change the mode used by a lock or filehandle.

    INPUTS
	type    - CHANGE_FH or CHANGE_LOCK.
	object  - Filehandle or lock.
	newmode - New mode (see <dos/dos.h>).

    RESULT
	!= 0 if all went well, otherwise 0. IoErr() gives additional
	information in the latter case.

    NOTES
	Since filehandles and locks are identical under AROS the type
	argument is ignored.

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
    struct FileHandle *fh = (struct FileHandle *)BADDR(object);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Convert Open() and Lock() constants to filehandler flags. */
    ULONG newflags, mask;

    if(newmode == MODE_OLDFILE || newmode == MODE_READWRITE ||
       newmode == ACCESS_READ)
    {
        newflags = 0;
        mask     = FMF_LOCK;
    }
    else if(newmode == MODE_NEWFILE || newmode == ACCESS_WRITE)
    {
        newflags = FMF_LOCK;
        mask     = FMF_LOCK;
    }
    else
    {
	if (newmode & FMF_APPEND)
	    fh->fh_Flags |= FHF_APPEND;
	else
	    fh->fh_Flags &= ~FHF_APPEND;

	newmode &= ~FMF_APPEND;

	newflags = newmode;
        mask     = 0xFFFFFFFF;
    }

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_FILE_MODE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_FILE_MODE.io_FileMode = newflags;
    iofs.io_Union.io_FILE_MODE.io_Mask     = mask;

    /* Send the request. */
    DoIO(&iofs.IOFS);

    /* Set error code and return */
    SetIoErr(iofs.io_DosError);

    return iofs.io_DosError == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* ChangeMode */
