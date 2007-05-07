/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */

#include <aros/debug.h>
#include <dos/notify.h>
#include <proto/dos.h>

	AROS_LH1(BOOL, StartNotify,

/*  SYNOPSIS */
	AROS_LHA(struct NotifyRequest *, notify, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 148, Dos)

/*  FUNCTION

    Send a notification request to a filesystem. You will then be notified
    whenever the file (or directory) changes.

    INPUTS

    notify  --  a notification request for the file or directory to monitor

    RESULT

    Success/failure indicator.

    NOTES

    The file or directory connected to a notification request does not have
    to exist at the time of calling StartNotify().
    The NotifyRequest used with this function should not be altered while
    active.

    EXAMPLE

    BUGS

    SEE ALSO

    EndNotify(), <dos/notify.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys iofs;
    struct FileHandle *dir;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_ADD_NOTIFY, DOSBase);

    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;

    notify->nr_MsgCount = 0;

    if (strchr(notify->nr_Name, ':') != NULL)
    {
	DoName(&iofs, notify->nr_Name, DOSBase);
    }
    else
    {
	dir = BADDR(CurrentDir(NULL));
	CurrentDir(MKBADDR(dir));		/* Set back the current dir */
	
	if (dir == NULL)
	{
	    return DOSFALSE;
	}
	
	iofs.IOFS.io_Device = dir->fh_Device;
	iofs.IOFS.io_Unit = dir->fh_Unit;
	
	/* Save device for EndNotify() purposes */
	notify->nr_Device = dir->fh_Device;
	
	if (iofs.IOFS.io_Device == NULL)
	{
	    return DOSFALSE;
	}

	DosDoIO(&iofs.IOFS);
    }

    SetIoErr(iofs.io_DosError);

    if (iofs.io_DosError != 0)
    {
	return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* StartNotify */
