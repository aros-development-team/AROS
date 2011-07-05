/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level filesystem access functions, packet version
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "fs_driver.h"

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, CONST_STRPTR name, LONG accessMode, struct DosLibrary *DOSBase)
{
    struct FileLock *fl = BADDR(parent);
    struct MsgPort *port;
    SIPTR error = 0;
    BSTR bstrname = C2BSTR(name);

    if (!bstrname)
    	return ERROR_NO_FREE_STORE;

    if (fl)
    	port = fl->fl_Task;
    else
    {
    	port   = dvp->dvp_Port;
    	parent = dvp->dvp_Lock;
    }

    *ret = (BPTR)dopacket3(DOSBase, &error, port, ACTION_LOCATE_OBJECT, parent, bstrname, accessMode);
    FREEC2BSTR(bstrname);

    return error;
}

LONG fs_Open(struct FileHandle *handle, UBYTE refType, BPTR lock, LONG mode, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    ULONG action;
    BSTR bstrname;
    struct MsgPort *port = NULL;
    struct Process *me;
    SIPTR error = 0;

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

    switch (refType)
    {
    case REF_LOCK:
	/* 'lock' parameter is actually a parent's BPTR lock */
    	port = ((struct FileLock *)BADDR(lock))->fl_Task;
    	break;

    case REF_DEVICE:
    	port = ((struct DevProc *)BADDR(lock))->dvp_Port;
    	lock = ((struct DevProc *)BADDR(lock))->dvp_Lock;
    	break;

    case REF_CONSOLE:
    	me = (struct Process *)FindTask(NULL);
    	port = me->pr_ConsoleTask;
    	/* console handler ACTION_FIND* ignores lock */
        break;
    }

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

    dopacket3(DOSBase, &error, port, action, MKBADDR(handle), lock, bstrname);
    FREEC2BSTR(bstrname);

    handle->fh_Type = port;
    return error;
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
    struct FileHandle *fh = BADDR(handle);

    dopacket3(DOSBase, &error, fh->fh_Type, ACTION_CHANGE_SIGNAL, fh->fh_Arg1, (IPTR)task, (SIPTR)NULL);

    return error;
}

LONG fs_AddNotify(struct NotifyRequest *notify, struct DevProc *dvp, BPTR lock, struct DosLibrary *DOSBase)
{
    SIPTR err = 0;
    LONG status = dopacket1(DOSBase, &err, notify->nr_Handler, ACTION_ADD_NOTIFY, (SIPTR)notify);

    return status ? 0 : err;
}

BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary *DOSBase)
{
    BPTR nfh;
    struct MsgPort *old;
    struct FileHandle *h;

    h = BADDR(fh);
    if (!h->fh_Type)
    	return Open("NIL:", mode);
    old = SetConsoleTask(h->fh_Type);
    nfh = Open("*", mode);
    SetConsoleTask(old);
    return nfh;
}
