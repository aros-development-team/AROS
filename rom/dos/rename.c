/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rename a file
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"
#include <string.h>

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
    struct Process *me = (struct Process *)FindTask(NULL);
    char buf1[256], vol[32];
    char buf2[256];
    int len;

    InitIOFS(&iofs, FSA_RENAME, DOSBase);

    len = SplitName(oldName, ':', vol, 0, sizeof(vol) - 1);

    if (len > 0)
    {
	/* get real name */
	BPTR lock = Lock(oldName, SHARED_LOCK);
	if (lock)
	{
	    if (NameFromLock(lock, buf1, sizeof(buf1) -1 ))
		oldName = buf1;
	    UnLock(lock);
	}
    }
    else
    {
	/* convert to absolute path */
	if (NameFromLock(me->pr_CurrentDir, buf1, sizeof(buf1) - 1))
	{
	    int namelen = strlen(oldName);
	    len = strlen(buf1);
	    if (len + namelen < sizeof(buf1))
	    {
		buf1[len++] = '/';
		CopyMem(oldName, buf1 + len, namelen);
		len += namelen;
		buf1[len] = '\0';
		oldName = buf1;
	    }
	}
    }

    len = SplitName(newName, ':', vol, 0, sizeof(vol) - 1);

    if (len <= 0)
    {
	/* convert to absolute path */
	if (NameFromLock(me->pr_CurrentDir, buf2, sizeof(buf2) - 1))
	{
	    int namelen = strlen(newName);
	    len = strlen(buf2);
	    if (len + namelen < sizeof(buf2))
	    {
		buf2[len++] = '/';
		CopyMem(newName, buf2 + len, namelen);
		len += namelen;
		buf2[len] = '\0';
		newName = buf2;
	    }
	}
    }

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
