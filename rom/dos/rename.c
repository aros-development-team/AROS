/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Rename a file
    Lang: english
*/

#include <aros/debug.h>
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
    LONG status;
    struct Process *me = (struct Process *)FindTask(NULL);
    char buf1[256], vol[32];
    char buf2[256];
    int len;
    BSTR bstrNewName, bstrOldName;

    len = SplitName(oldName, ':', vol, 0, sizeof(vol) - 1);

    if (len > 0)
    {
	/* get real name */
	BPTR lock = Lock(oldName, SHARED_LOCK);
	if (lock)
	{
	    if (NameFromLock(lock, buf1, sizeof(buf1) - 1))
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
	    if (len + namelen < sizeof(buf1) - 1)
	    {
		if (buf1[len - 1] != ':')
		    buf1[len++] = '/';
		CopyMem(oldName, buf1 + len, namelen + 1);
		oldName = buf1;
	    }
	}
    }

    len = SplitName(newName, ':', vol, 0, sizeof(vol) - 1);

    if (len > 0)
    {
	/* get real name of destination path */
	BPTR lock;
	char *pos;
	const char *pos2;

	strcpy(buf2, newName);
	pos = strrchr(buf2, '/');
	if (!pos)
	{
	    pos = buf2 + len;
	    *pos = '\0';
	}
	else
	    *pos++ = '\0';
	
	lock = Lock(buf2, SHARED_LOCK);
	if (lock)
	{
	    if (NameFromLock(lock, buf2, sizeof(buf2) - 1))
	    {
		int namelen;
		len = strlen(buf2);
		pos2 = newName + (int)(pos - buf2);
		namelen = strlen(pos2);
		if (len + namelen < sizeof(buf2) - 1)
		{
		    if (buf2[len - 1] != ':')
			buf2[len++] = '/';
		    CopyMem(pos2, buf2 + len, namelen + 1);
		    newName = buf2;
		}
	    }
	    UnLock(lock);
	}
    }
    else
    {
	/* convert to absolute path */
	if (NameFromLock(me->pr_CurrentDir, buf2, sizeof(buf2) - 1))
	{
	    int namelen = strlen(newName);
	    len = strlen(buf2);
	    if (len + namelen < sizeof(buf2) - 1)
	    {
		if (buf2[len - 1] != ':')
		    buf2[len++] = '/';
		CopyMem(newName, buf2 + len, namelen + 1);
		newName = buf2;
	    }
	}
    }

    D(bug("[Dos] rename %s %s\n", oldName, newName));

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

    bstrNewName = C2BSTR(newName);
    bstrOldName = C2BSTR(oldName);
    status = dopacket4(DOSBase, NULL, olddvp->dvp_Port, ACTION_RENAME_OBJECT, olddvp->dvp_Lock, bstrOldName, newdvp->dvp_Lock, bstrNewName);
    FREEC2BSTR(bstrOldName);
    FREEC2BSTR(bstrNewName);

    FreeDeviceProc(olddvp);
    FreeDeviceProc(newdvp);

    return status;

    AROS_LIBFUNC_EXIT
} /* Rename */
