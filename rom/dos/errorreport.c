/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <exec/ports.h>
#include <aros/debug.h>

#define CATCOMP_NUMBERS

#include "dos_intern.h"
#include "strings.h"

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
    Displays a requester with Retry/Cancel buttons for an error.
    IoErr() is set to "code".
        
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
    DOSFALSE - user has selected "Retry"
    DOSTRUE  - user has selected "Cancel" or code wasn't understood or
               pr_WindowPtr is -1 or if an attempt to open the requester fails.

    NOTES

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
    char                buf[128];
    struct DevProc      *dvp;
    struct DosList      *dol;
    APTR                args[2];
    ULONG               idcmp = 0;
    LONG                err;
    LONG                res;
    struct MsgPort      *msgport;
    struct PacketHelperStruct phs;
    
    ASSERT_VALID_PROCESS(me);

    /* do nothing if errors are disabled */
    if (me->pr_WindowPtr == (APTR) -1) {
        SetIoErr(code);
        return DOSTRUE;
    }
    buf[0] = 0;

    /* first setup the error format and work out which args we need */
    switch (code) {
        /* Volume FOO: is not validated */
        case ERROR_DISK_NOT_VALIDATED:
            format = DosGetString(MSG_STRING_DISK_NOT_VALIDATED);
            want_volume = TRUE;
            break;

        /* Volume FOO: is write protected */
        case ERROR_DISK_WRITE_PROTECTED:
            format = DosGetString(MSG_STRING_DISK_WRITE_PROTECTED);
            want_volume = TRUE;
            break;

        /* Please (insert|replace) volume FOO: in ... */
        case ERROR_DEVICE_NOT_MOUNTED:
            if (type == REPORT_INSERT) {
                format = DosGetString(MSG_STRING_DEVICE_NOT_MOUNTED_INSERT);
                want_volume = TRUE;
            }
            else if (type == REPORT_STREAM) {
                format = DosGetString(MSG_STRING_DEVICE_NOT_MOUNTED_REPLACE_TARGET);
                want_volume = want_device = TRUE;
            }
            else {
                format = DosGetString(MSG_STRING_DEVICE_NOT_MOUNTED_REPLACE);
                want_volume = TRUE;
            }
            idcmp = IDCMP_DISKINSERTED;
            break;

        /* Volume FOO: is full */
        case ERROR_DISK_FULL:
            format = DosGetString(MSG_STRING_DISK_FULL);
            want_volume = TRUE;
            break;

        /* Not a DOS disk in ...*/
        case ERROR_NOT_A_DOS_DISK:
            format = DosGetString(MSG_STRING_NOT_A_DOS_DISK);
            want_device = TRUE;
            break;

        /* No disk present in ...*/
        case ERROR_NO_DISK:
            format = DosGetString(MSG_STRING_NO_DISK);
            want_device = TRUE;
            break;

        /* You MUST replace volume FOO: in ... */
        case ABORT_BUSY:
            format = DosGetString(MSG_STRING_ABORT_BUSY);
            want_volume = want_device = TRUE;
            idcmp = IDCMP_DISKINSERTED;
            break;

        /* Volume FOO: has a read/write error */
        case ABORT_DISK_ERROR:
            format = DosGetString(MSG_STRING_ABORT_DISK_ERROR);
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
            if (arg1 == (IPTR)NULL)
                return DOSTRUE;
            msgport = ((struct FileHandle *) BADDR(arg1))->fh_Type;
            dl = (struct DeviceList*)BADDR(dopacket1(DOSBase, NULL, msgport, ACTION_CURRENT_VOLUME, ((struct FileHandle *) BADDR(arg1))->fh_Arg1));
            if (dl)
                volname =  AROS_BSTR_ADDR(dl->dl_Name);
            break;
            
        case REPORT_TASK:
            /* XXX what is this? */
            return DOSTRUE;

        /* a lock */
        case REPORT_LOCK:
        {
            struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, 0);
            if (!fib)
                return DOSTRUE;
            /* if they provided a lock, just use it */
            if (arg1 != (IPTR)NULL) {
                msgport = ((struct FileLock *) BADDR(arg1))->fl_Task;
            } else {
                msgport = device;
            }
            if (dopacket2(DOSBase, NULL, msgport, ACTION_EXAMINE_OBJECT, arg1, (SIPTR)MKBADDR(fib))) {
                fixfib(fib);
                strncpy(buf, fib->fib_FileName, sizeof (buf) - 1);
                buf[sizeof(buf) - 1] = 0;
            }
            FreeDosObject(DOS_FIB, fib);
            if (buf[0] == 0)
                return DOSTRUE;
            volname = buf;
        }
        break;

        /* a volume, ie a DeviceList */
        case REPORT_VOLUME:
            if (arg1 == (IPTR)NULL)
                return DOSTRUE;

            dl = (struct DeviceList *) arg1;
            volname = AROS_BSTR_ADDR(dl->dl_Name);
            msgport = dl->dl_Task;
            break;
            
        /* raw volume name */
        case REPORT_INSERT:
            if (arg1 == (IPTR)NULL)
                return DOSTRUE;
            if (!getpacketinfo(DOSBase, (STRPTR)arg1, &phs))
                return DOSTRUE;
            msgport = phs.port;
            volname = (STRPTR) arg1;
            /* rip off any trailing stuff, if its there */
            if (SplitName(volname, ':', buf, 0, sizeof(buf)-1) == -1)
                volname = buf;
            freepacketinfo(DOSBase, &phs);
            break;

        /* do nothing with other types */
        default:
            return DOSTRUE;
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

        /* search the list for a device node with the same port as the volume */
        dol = LockDosList(LDF_READ | LDF_ALL);
        while (dol != NULL && (dol->dol_Type != DLT_DEVICE || dol->dol_Task != msgport))
            dol = BADDR(dol->dol_Next);

        /* found it */
        if (dol != NULL)
            devname = (char*)dol->dol_Name + 1;

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
