/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rename a file
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

	AROS_LH2(LONG, Rename,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, oldName, D1),
	AROS_LHA(CONST_STRPTR, newName, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 13, Dos)

/*  FUNCTION
	Renames a given file. The old name and the new name must point to the
	same volume.

    INPUTS
	oldName - Name of the file to rename
	newName - New name of the file to rename

    RESULT
	boolean - DOSTRUE or DOSFALSE. IoErr() provides additional information
	on DOSFALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Calls the action FSA_RENAME on the filesystem-handler.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    LONG error;
    struct Device *olddev, *newdev;

    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_RENAME, DOSBase);

    error = DevName(oldName, &olddev, DOSBase);
    if(error != 0)
    {
	SetIoErr(error);
	return DOSFALSE;
    }

    error = DevName(newName, &newdev, DOSBase);
    if(error != 0)
    {
	SetIoErr(error);
	return DOSFALSE;
    }

    if(olddev != newdev)
    {
	SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
	return DOSFALSE;
    }

    iofs.io_Union.io_RENAME.io_Filename = oldName;
    iofs.io_Union.io_RENAME.io_NewName = newName;

    DoName(&iofs, oldName, DOSBase);

    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError)
    {
	return DOSFALSE;
    }
    else
    {
	return DOSTRUE;
    }

    AROS_LIBFUNC_EXIT
} /* Rename */
