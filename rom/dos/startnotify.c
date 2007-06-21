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

    /* set up some defaults */
    notify->nr_MsgCount = 0;
    notify->nr_FullName = NULL;

    /* turn the filename into a device and dir lock */
    if ((dvp = GetDeviceProc(notify->nr_Name, NULL)) == NULL)
        return DOSFALSE;

    /* setup the request */
    InitIOFS(&iofs, FSA_ADD_NOTIFY, DOSBase);
    iofs.io_Union.io_NOTIFY.io_NotificationRequest = notify;

    iofs.IOFS.io_Device = (struct Device *) dvp->dvp_Port;

    /* remember the handler for EndNotify() (but see the comments there about
     * why we don't really use it */
    notify->nr_Handler = dvp->dvp_Port;

    /* if no lock is returned by GetDeviceProc() (eg if the path is for a
     * device or volume), then the path is fine as-is, and just use the
     * device/volume root lock */
    if (dvp->dvp_Lock == NULL) {
        notify->nr_FullName = notify->nr_Name;
        iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;
    }

    /* otherwise we need to expand the name using the lock */
    else {
        /* get the name */
        if (NameFromLock(dvp->dvp_Lock, buf, sizeof(buf)) == DOSFALSE) {
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }
        len = strlen(buf);

        /* if its not some absolute base thing, then add a dir seperator for
         * the concat operation below */
        if (buf[len-1] != ':') {
            buf[len] = '/';
            len++;
        }

        /* look for the ':' following the assign name in the path provided by
         * the caller */
        p = notify->nr_Name;
        while (*p && *p != ':')
            p++;

        /* if we found it, move past it */
        if (*p)
            p++;

        /* hit the end, so the name is a relative path, and we take all of it */
        else
            p = notify->nr_Name;

        len2 = strlen(p);

        if ((buf2 = AllocVec(len + len2 + 1, MEMF_PUBLIC)) == NULL) {
            FreeDeviceProc(dvp);
            SetIoErr(ERROR_NO_FREE_STORE);
            return DOSFALSE;
        }

        /* concatenate the two bits */
        CopyMem(buf, buf2, len);
        CopyMem(p, buf2 + len, len2 + 1);

        /* thats our expanded name */
        notify->nr_FullName = buf2;

        /* use the assign base lock as the relative lock */
        iofs.IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;
    }

    /* done with this */
    FreeDeviceProc(dvp);

    /* send the request, with error reporting */
    do {
        DosDoIO(&iofs.IOFS);
    } while (iofs.io_DosError != 0 && ErrorReport(iofs.io_DosError, REPORT_LOCK, 0, dvp->dvp_Port));

    SetIoErr(iofs.io_DosError);

    /* something broke, clean up */
    if (iofs.io_DosError != 0) {
        if (notify->nr_FullName != notify->nr_Name)
            FreeVec(notify->nr_FullName);
        return DOSFALSE;
    }

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* StartNotify */
