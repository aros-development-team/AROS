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
    struct DevProc *olddvp, *newdvp;
    struct IOFileSys iofs;
    LONG err;

    InitIOFS(&iofs, FSA_RENAME, DOSBase);

    /* get device pointers */
    if ((olddvp = GetDeviceProc(oldName, NULL)) == NULL ||
        (newdvp = GetDeviceProc(newName, NULL)) == NULL) {
        FreeDeviceProc(olddvp);
        return DOSFALSE;
    }

    /* make sure they're on the same device
     * XXX this is insufficient, see comments in samedevice.c */
    if (olddvp->dvp_Port != newdvp->dvp_Port) {
        FreeDeviceProc(olddvp);
        FreeDeviceProc(newdvp);
        SetIoErr(ERROR_RENAME_ACROSS_DEVICES);
        return DOSFALSE;
    }

    iofs.io_Union.io_RENAME.io_NewName = StripVolume(newName);
    err = DoIOFS(&iofs, olddvp, oldName, DOSBase);

    FreeDeviceProc(olddvp);
    FreeDeviceProc(newdvp);

    return err == 0 ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* Rename */
