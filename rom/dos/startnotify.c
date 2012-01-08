/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <dos/notify.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include <string.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */

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

    LONG err;
    struct DevProc *dvp;
    UBYTE buf[MAXFILENAMELENGTH+1], *buf2, *p;
    ULONG len, len2;
    BPTR lock = BNULL;

    /* set up some defaults */
    notify->nr_MsgCount = 0;
    notify->nr_FullName = NULL;

    D(bug("[StartNotify] Request 0x%p, Name %s\n", notify, notify->nr_Name));

    /* turn the filename into a device and dir lock */
    if ((dvp = GetDeviceProc(notify->nr_Name, NULL)) == NULL)
        return DOSFALSE;

    /* remember the handler for EndNotify() */
    notify->nr_Handler = dvp->dvp_Port;

    /* if no lock is returned by GetDeviceProc() (eg if the path is for a
     * device or volume root), then get the handler to resolve the name of the
     * device root lock */
    if (dvp->dvp_Lock == BNULL)
    {
        UBYTE name[MAXFILENAMELENGTH+1], *src, *dst;
        struct FileInfoBlock *fib;

        src = notify->nr_Name;
        dst = name;

        while (*src != ':')
            *dst++ = *src++;

        *dst++ = ':';
        *dst++ = '\0';

        if ((fib = AllocDosObject(DOS_FIB, NULL)) == NULL) {
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }

        if((lock = Lock(name, SHARED_LOCK)) == BNULL) {
            FreeDosObject(DOS_FIB, fib);
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }

        if (!Examine(lock, fib)) {
            FreeDosObject(DOS_FIB, fib);
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }

        /* copy it to our processing buffer */
        src = fib->fib_FileName;
        dst = buf;

        while (*src != '\0')
            *dst++ = *src++;

        *dst++ = ':';
        *dst++ = '\0';

        FreeDosObject(DOS_FIB, fib);
    }

    /* otherwise we need to expand the name using the lock */
    else
    {
        /* get the name */
        if (NameFromLock(dvp->dvp_Lock, buf, sizeof(buf)) == DOSFALSE)
        {
            FreeDeviceProc(dvp);
            return DOSFALSE;
        }
    }

    len = strlen(buf);

    /* if it's not some absolute base thing, then add a dir seperator for
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

    if ((buf2 = AllocVec(len + len2 + 1, MEMF_PUBLIC)) == NULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);

        /* cleanup */
        if (lock != BNULL)
            UnLock(lock);
        FreeDeviceProc(dvp);

        return DOSFALSE;
    }

    /* concatenate the two bits */
    CopyMem(buf, buf2, len);
    CopyMem(p, buf2 + len, len2 + 1);

    /* that's our expanded name */
    notify->nr_FullName = buf2;

    /* send the request, with error reporting */
    do
    {
    	err = fs_AddNotify(notify, dvp, lock, DOSBase);
    } while (err != 0 && ErrorReport(err, REPORT_LOCK, 0, notify->nr_Handler) == DOSFALSE);

    /* cleanup */
    if (lock != BNULL)
        UnLock(lock);
    FreeDeviceProc(dvp);

    /* something broke, clean up */
    if (err != 0)
    {
        if (notify->nr_FullName != notify->nr_Name)
        {
            FreeVec(notify->nr_FullName);
            notify->nr_FullName = NULL;
        }

        SetIoErr(err);
        return DOSFALSE;
    }

    D(bug("[StartNotify] nr_Handler 0x%p\n", notify->nr_Handler));

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* StartNotify */
