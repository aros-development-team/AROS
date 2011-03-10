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
    LONG error = 0;
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

    *ret = dopacket3(DOSBase, &error, port, ACTION_LOCATE_OBJECT, parent, bstrname, accessMode);
    FreeVec(BADDR(bstrname));

    return error;
}

LONG fs_Open(struct FileHandle *handle, UBYTE refType, BPTR lock, LONG mode, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    ULONG action;
    BSTR bstrname;
    struct MsgPort *port = NULL;
    struct Process *me;
    LONG error = 0;

    if (mode == MODE_READWRITE)
	action = ACTION_FINDUPDATE;
    else if (mode == MODE_NEWFILE)
	action = ACTION_FINDOUTPUT;
    else if (mode == MODE_OLDFILE)
	action = ACTION_FINDINPUT;
    else if (mode & FMF_CREATE)
	action = ACTION_FINDOUTPUT;
    else if (mode & FMF_CLEAR)
	action = ACTION_FINDOUTPUT;
    else if (mode & (FMF_READ | FMF_WRITE))
	action = ACTION_FINDINPUT;
    else
    {
	bug("unknown access mode %x\n", mode);
	return ERROR_ACTION_NOT_KNOWN;
    }

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
        /* CHECKME: 'lock' being passed is process' console file handle, not a lock. Is it correct to pass it on ? */
        break;
    }

    if (!port)
    {
    	/* was NIL: */
    	SetIoErr(0);
    	handle->fh_Type = BNULL;
        return DOSTRUE;
    }

    bstrname = C2BSTR(name);
    if (!bstrname)
    	return ERROR_NO_FREE_STORE;

    dopacket3(DOSBase, &error, port, action, MKBADDR(handle), lock, bstrname);
    FreeVec(BADDR(bstrname));

    handle->fh_Type = port;
    return error;
}

LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase)
{
    LONG error = 0;
    struct FileHandle *fh = BADDR(handle);

    dopacket3(DOSBase, &error, fh->fh_Type, ACTION_CHANGE_SIGNAL, fh->fh_Arg1, (IPTR)task, (SIPTR)NULL);

    return error;
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
