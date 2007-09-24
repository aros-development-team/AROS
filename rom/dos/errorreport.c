/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <exec/ports.h>

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(BOOL, ErrorReport,

/*  SYNOPSIS */
	AROS_LHA(LONG            , code  , D1),
	AROS_LHA(LONG            , type  , D2),
	AROS_LHA(IPTR            , arg1  , D3),
	AROS_LHA(struct MsgPort *, device, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 80, Dos)

/*  FUNCTION

    INPUTS

    code    --  The error to put up the requester for
    type    --  Type of request

                REPORT_LOCK    --  arg1 is a lock (BPTR).
                REPORT_FH      --  arg1 is a filehandle (BPTR).
		REPORT_VOLUME  --  arg1 is a volumenode (C pointer).
		REPORT_INSERT  --  arg1 is the string for the volumename

    arg1    --  Argument according to type (see above)
    device  --  Optional handler task address (obsolete!)

    RESULT

    NOTES

    Locks and filehandles are the same in AROS so there is redundancy in
    the parameters. Furthermore, the 'device' argument is not cared about
    as AROS don't build filesystems via handlers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process      *me = (struct Process *) FindTask(NULL);
    STRPTR              format;
    BOOL                want_volume = FALSE;
    BOOL                want_device = FALSE;
    STRPTR              volname = NULL;
    STRPTR              devname = NULL;
    struct DeviceList   *dl = NULL;
    struct Device       *handler = NULL;
    struct Unit         *unit = NULL;
    char                buf[128];
    struct DevProc      *dvp;
    struct DosList      *dol;
    APTR                args[2];
    ULONG               idcmp = 0;
    LONG                err;
    LONG                res;

    /* do nothing if errors are disabled */
    if (me->pr_WindowPtr == (APTR) -1)
        return DOSTRUE;

    /* first setup the error format and work out which args we need */
    switch (code) {
        /* Volume FOO: is not validated */
        case ERROR_DISK_NOT_VALIDATED:
            format = DosGetString(STRING_DISK_NOT_VALIDATED);
            want_volume = TRUE;
            break;

        /* Volume FOO: is write protected */
        case ERROR_DISK_WRITE_PROTECTED:
            format = DosGetString(STRING_DISK_WRITE_PROTECTED);
            want_volume = TRUE;
            break;

        /* Please (insert|replace) volume FOO: in ... */
        case ERROR_DEVICE_NOT_MOUNTED:
            if (type == REPORT_INSERT) {
                format = DosGetString(STRING_DEVICE_NOT_MOUNTED_INSERT);
                want_volume = TRUE;
            }
            else if (type == REPORT_STREAM) {
                format = DosGetString(STRING_DEVICE_NOT_MOUNTED_REPLACE_TARGET);
                want_volume = want_device = TRUE;
            }
            else {
                format = DosGetString(STRING_DEVICE_NOT_MOUNTED_REPLACE);
                want_volume = TRUE;
            }
            idcmp = IDCMP_DISKINSERTED;
            break;

        /* Volume FOO: is full */
        case ERROR_DISK_FULL:
            format = DosGetString(STRING_DISK_FULL);
            want_volume = TRUE;
            break;

        /* Not a DOS disk in ...*/
        case ERROR_NOT_A_DOS_DISK:
            format = DosGetString(STRING_NOT_A_DOS_DISK);
            want_device = TRUE;
            break;

        /* No disk present in ...*/
        case ERROR_NO_DISK:
            format = DosGetString(STRING_NO_DISK);
            want_device = TRUE;
            break;

        /* You MUST replace volume FOO: in ... */
        case ABORT_BUSY:
            format = DosGetString(STRING_ABORT_BUSY);
            want_volume = want_device = TRUE;
            idcmp = IDCMP_DISKINSERTED;
            break;

        /* Volume FOO: has a read/write error */
        case ABORT_DISK_ERROR:
            format = DosGetString(STRING_ABORT_DISK_ERROR);
            want_volume = TRUE;
            break;

        /* do nothing with other errors */
        default:
            return DOSTRUE;
    }

    /* now we need to determine the volume name. if they gave it to use
     * (REPORT_INSERT), we just use it. otherwise, we get it from the device
     * list (REPORT_VOLUME). if we don't have one, we use the handler/unit
     * pair to find it */
    switch (type) {
        /* a filehandle */
        case REPORT_STREAM:
            if (arg1 == NULL)
                return DOSTRUE;

            handler = ((struct FileHandle *) BADDR(arg1))->fh_Device;
            unit = ((struct FileHandle *) BADDR(arg1))->fh_Unit;
            break;
            
        case REPORT_TASK:
            /* XXX what is this? */
            return DOSTRUE;

        /* a lock */
        case REPORT_LOCK:
            /* if they provided a lock, just use it */
            if (arg1 != NULL) {
                handler = ((struct FileHandle *) BADDR(arg1))->fh_Device;
                unit = ((struct FileHandle *) BADDR(arg1))->fh_Unit;
            }

            /* otherwise we use the secondary device, and we look through the
             * doslist to determine the unit */
            else {
                handler = (struct Device *) device;

                /* find the doslist entry */
                dol = LockDosList(LDF_READ | LDF_ALL);
                while (dol != NULL && (dol->dol_Type != DLT_VOLUME ||
                                       dol->dol_Ext.dol_AROS.dol_Device != handler))
                    dol = dol->dol_Next;

                /* found it, steal its unit */
                if (dol != NULL)
                    unit = dol->dol_Ext.dol_AROS.dol_Unit;
                
                UnLockDosList(LDF_READ | LDF_ALL);

                /* if we didn't find it, there's not much more we can do */
                if (dol == NULL)
                    return DOSTRUE;
            }

            break;

        /* a volume, ie a DeviceList */
        case REPORT_VOLUME:
            if (arg1 == NULL)
                return DOSTRUE;

            dl = (struct DeviceList *) arg1;
            break;
            
        /* raw volume name */
        case REPORT_INSERT:
            if (arg1 == NULL)
                return DOSTRUE;

            volname = (STRPTR) arg1;
            /* rip off any trailing stuff, if its there */
            if (SplitName(volname, ':', buf, 0, sizeof(buf)-1) == -1)
                volname = buf;
            break;

        /* do nothing with other types */
        default:
            return DOSTRUE;
    }

    /* get the name if we don't already have it */
    if (volname == NULL) {

        /* just use the volume pointer if we already have it */
        if (dl != NULL)
            volname = dl->dl_Ext.dl_AROS.dol_DevName;

        /* otherwise we have to get it from the handler */
        else {
            /* XXX for packets we'd just call ACTION_CURRENT_DEVICE */

            struct FileHandle *fh;
            char *p;

            /* remember the current error just in case this fails. I don't know if
             * this is actually necessary but I'm trying to keep side-effects to a
             * minimum */
            err = IoErr();

            /* make a fake lock (filehandle) */
            if ((fh = AllocDosObject(DOS_FILEHANDLE, 0)) == NULL) {
                SetIoErr(err);
                return DOSTRUE;
            }

            fh->fh_Device = handler;
            fh->fh_Unit = unit;

            /* get the handler to give us the name */
            if (NameFromLock(MKBADDR(fh), buf, 127) != DOSTRUE) {
                FreeDosObject(fh, DOS_FILEHANDLE);
                SetIoErr(err);
                return DOSTRUE;
            }

            /* cleanup */
            FreeDosObject(fh, DOS_FILEHANDLE);
            SetIoErr(err);

            /* find the volume seperator */
            for (p = buf; *p != ':' && *p != '\0'; p++);
        
            /* not there. can this happen? */
            if (*p == '\0')
                return DOSTRUE;

            /* overwrite it, and we have a volume name */
            *p = '\0';

            volname = buf;
        }
    }

    /* for the device name we need the doslist entry */
    if (want_device) {
        /* XXX for packets we just search the doslist for a DLT_DEVICE with
         * the same task pointer. no need to worry about multiple units */

        /* remember the current error in case we have to bail out */
        err = IoErr();

        /* get the entry for the volume */
        if ((dvp = GetDeviceProc(volname, NULL)) == NULL) {
            SetIoErr(err);
            return DOSTRUE;
        }

        /* search the list for a device node with the same handler/port as the
         * volume */
        dol = LockDosList(LDF_READ | LDF_ALL);
        while (dol != NULL && (dol->dol_Type != DLT_DEVICE ||
                               dol->dol_Ext.dol_AROS.dol_Device != (struct Device *) dvp->dvp_Port ||
                               dol->dol_Ext.dol_AROS.dol_Unit != dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit))
            dol = dol->dol_Next;

        /* found it */
        if (dol != NULL)
            devname = dol->dol_Ext.dol_AROS.dol_DevName;

        /* XXX can this happen? */
        else
            devname = "???";

        UnLockDosList(LDF_READ | LDF_ALL);

        FreeDeviceProc(dvp);
    }

    /* have all we need, now to build the arg array */
    if (want_volume) {
        args[0] = volname;
        if (want_device)
            args[1] = devname;
    }
    else if (want_device)
        args[0] = devname;
    
    /* display it. idcmp is set further up to catch "disk insert" events if
     * we're waiting for them to insert something */

    res = DisplayError(format, idcmp, &args);

    SetIoErr(code);
   
    return res == 0 ? DOSFALSE : DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ErrorReport */
