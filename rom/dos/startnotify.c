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
    struct DevProc *dvp;
    UBYTE buf[256], *buf2, *p;
    ULONG len, len2;

    notify->nr_MsgCount = 0;
    notify->nr_FullName = NULL;

    if ((dvp = GetDeviceProc(notify->nr_Name, NULL)) == NULL)
        return DOSFALSE;

    InitIOFS(&iofs, FSA_ADD_NOTIFY, DOSBase);
    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;

    iofs.IOFS.io_Device = (struct Device *) dvp->dvp_Port;

    notify->nr_Device = (struct Device *) dvp->dvp_Port;
    notify->nr_FullName = notify->nr_Name;

    if (dvp->dvp_Lock != NULL) {
        if (NameFromLock(dvp->dvp_Lock, buf, sizeof(buf)) == DOSFALSE) {
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }
        len = strlen(buf);

        if (buf[len-1] != ':') {
            buf[len] = '/';
            len++;
        }

        p = notify->nr_Name;
        while (*p && *p != ':')
            p++;

        if (*p)
            p++;
        else
            p = notify->nr_Name;

        len2 = strlen(p);

        if ((buf2 = AllocVec(len + len2 + 1, MEMF_PUBLIC)) == NULL) {
            FreeDeviceProc(dvp);
            SetIoErr(ERROR_NO_FREE_STORE);
            return DOSFALSE;
        }

        CopyMem(buf, buf2, len);
        CopyMem(p, buf2 + len, len2 + 1);
        notify->nr_FullName = buf2;

        iofs.IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;
    }

    else
        iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;

    FreeDeviceProc(dvp);

    DosDoIO(&iofs.IOFS);

    SetIoErr(iofs.io_DosError);

    if (iofs.io_DosError != 0) {
        if (notify->nr_FullName != notify->nr_Name)
            FreeVec(notify->nr_FullName);
        return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* StartNotify */
