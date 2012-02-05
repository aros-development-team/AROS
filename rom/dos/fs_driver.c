/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level filesystem access functions, packet version
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "fs_driver.h"

LONG fs_LocateObject(BPTR *ret, struct MsgPort *port, BPTR parent, CONST_STRPTR name, LONG accessMode, struct DosLibrary *DOSBase)
{
    SIPTR error = 0;
    BSTR bstrname = C2BSTR(name);

    if (!bstrname)
    	return ERROR_NO_FREE_STORE;

    *ret = (BPTR)dopacket3(DOSBase, &error, port, ACTION_LOCATE_OBJECT, parent, bstrname, accessMode);
    FREEC2BSTR(bstrname);

    return (*ret) ? 0 : error;
}

LONG fs_Open(struct FileHandle *handle, struct MsgPort *port, BPTR lock, LONG mode, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    ULONG action;
    BSTR bstrname;
    SIPTR error = 0;
    LONG status;

    /* The MODE_* flags exactly match their corresponding ACTION_*
     * flags:
     *
     * MODE_READWRITE = ACTION_FINDUPDATE = 1004
     * MODE_OLDFILE   = ACTION_FINDINPUT  = 1005
     * MODE_NEWFILE   = ACTION_FINDOUTPUT = 1006
     *
     * Even so, we don't want bad data propogaing
     * down, so check that the mode is valid.
     */
    if ((mode == MODE_READWRITE) ||
        (mode == MODE_OLDFILE) ||
        (mode == MODE_NEWFILE))
        action = mode;
    else
        return ERROR_NOT_IMPLEMENTED;

    if (!port)
    {
    	/* handler pointer not set, return NIL: handle */
    	SetIoErr(0);
    	handle->fh_Type = BNULL;
    	/* NIL: is considered interactive */
    	handle->fh_Interactive = DOSTRUE;
    	return 0;
    }

    bstrname = C2BSTR(name);
    if (!bstrname)
    	return ERROR_NO_FREE_STORE;

    status = dopacket3(DOSBase, &error, port, action, MKBADDR(handle), lock, bstrname);
    FREEC2BSTR(bstrname);

    handle->fh_Type = port;
    return status ? 0 : error;
}

LONG fs_ReadLink(BPTR parent, struct DevProc *dvp, CONST_STRPTR path, STRPTR buffer, ULONG size, struct DosLibrary *DOSBase)
{
    struct MsgPort *port;

    if (parent)
    {
	struct FileLock *fl = BADDR(parent);

    	port = fl->fl_Task;
    }
    else
    {
    	port   = dvp->dvp_Port;
    	parent = dvp->dvp_Lock;
    }

    return ReadLink(port, parent, path, buffer, size);
}

LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase)
{
    SIPTR error = 0;
    LONG status;
    struct FileHandle *fh = BADDR(handle);

    status = dopacket3(DOSBase, &error, fh->fh_Type, ACTION_CHANGE_SIGNAL, fh->fh_Arg1, (IPTR)task, (SIPTR)NULL);

    return status ? 0 : error;
}

LONG fs_AddNotify(struct NotifyRequest *notify, struct DevProc *dvp, BPTR lock, struct DosLibrary *DOSBase)
{
    SIPTR err = 0;
    LONG status = dopacket1(DOSBase, &err, notify->nr_Handler, ACTION_ADD_NOTIFY, (SIPTR)notify);

    return status ? 0 : err;
}
