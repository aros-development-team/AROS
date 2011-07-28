/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#include "dos_intern.h"

BOOL getpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR name, struct PacketHelperStruct *phs)
{
    if (!strchr(name, ':'))
    {
    	/* no ":" */
    	struct Process *me = (struct Process *)FindTask(NULL);
        BPTR cur;
    	BSTR bstrname = C2BSTR(name);
        struct FileLock *fl;
        cur = me->pr_CurrentDir;
        if (!cur)
            cur = DOSBase->dl_SYSLock;
        fl = BADDR(cur);
        phs->port = fl ? fl->fl_Task : NULL;
        phs->lock = cur;
        phs->dp = NULL;
        phs->name = bstrname;
        return TRUE;
    } else { /* ":" */
    	BSTR bstrname = C2BSTR(name);
        struct DevProc *dvp = NULL;
        if ((dvp = GetDeviceProc(name, dvp))) {
            phs->name = bstrname;
            phs->port = dvp->dvp_Port;
            phs->lock = dvp->dvp_Lock;
            phs->dp = dvp;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL getdevpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR devname, CONST_STRPTR name, struct PacketHelperStruct *phs)
{
    if ((phs->dp = GetDeviceProc(devname, NULL)) == NULL)
        return DOSFALSE;
    /* we're only interested in real devices */
    if (phs->dp->dvp_DevNode->dol_Type != DLT_DEVICE) {
        FreeDeviceProc(phs->dp);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return DOSFALSE;
    }
    phs->port = phs->dp->dvp_Port;
    phs->lock = BNULL;
    phs->name = BNULL;
    if (!name)
    	return TRUE;
    phs->name = C2BSTR(name);
    if (!phs->name) {
        FreeDeviceProc(phs->dp);
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    return TRUE;
}

void freepacketinfo(struct DosLibrary *DOSBase, struct PacketHelperStruct *phs)
{
    if (phs->dp)
    	FreeDeviceProc(phs->dp);
    FREEC2BSTR(phs->name);
}
