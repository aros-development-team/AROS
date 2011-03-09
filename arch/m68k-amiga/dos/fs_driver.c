/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

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

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, STRPTR name, LONG accessMode, struct DosLibrary *DOSBase)
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
