/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level filesystem access functions, IOFS version
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>

#include "dos_intern.h"
#include "fs_driver.h"

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, STRPTR name, LONG accessMode, struct DosLibrary *DOSBase)
{
    struct FileHandle *fh = BADDR(parent);
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_OPEN, DOSBase);    

    switch (accessMode)
    {
    	case EXCLUSIVE_LOCK:
    	    iofs.io_Union.io_OPEN.io_FileMode = FMF_LOCK | FMF_READ;
    	    break;
    
    	case SHARED_LOCK:
    	    iofs.io_Union.io_OPEN.io_FileMode = FMF_READ;
    	    break;
    
    	default:
	    D(bug("[LocateObject] incompatible mode %d\n", accessMode));
	    return ERROR_ACTION_NOT_KNOWN;
    }

    iofs.io_Union.io_OPEN.io_Filename = name;

    if (fh)
    {
    	iofs.IOFS.io_Device = fh->fh_Device;
    	iofs.IOFS.io_Unit   = fh->fh_Unit;
    }
    else
    {
	iofs.IOFS.io_Device = (struct Device *)dvp->dvp_Port;

	if (dvp->dvp_Lock != BNULL)
	    iofs.IOFS.io_Unit = ((struct FileHandle *)BADDR(dvp->dvp_Lock))->fh_Unit;
	else
            iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;
    }

    DosDoIO(&iofs.IOFS);
    
    if (!iofs.io_DosError)
    {
        /* Create filehandle */
	struct FileHandle *handle = AllocDosObject(DOS_FILEHANDLE, NULL);

	handle->fh_Device = iofs.IOFS.io_Device;
	handle->fh_Unit   = iofs.IOFS.io_Unit;
	
	*ret = MKBADDR(handle);
    }

    return iofs.io_DosError;
}

LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase)
{
    struct IOFileSys iofs;
    struct FileHandle *fh = BADDR(handle);

    InitIOFS(&iofs, FSA_CHANGE_SIGNAL, DOSBase);

    iofs.io_Union.io_CHANGE_SIGNAL.io_Task = (struct Task *)task;

    iofs.IOFS.io_Device  = fh->fh_Device;
    iofs.IOFS.io_Unit    = fh->fh_Unit;

    DosDoIO(&iofs.IOFS);
    
    return iofs.io_DosError;
}

BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary *DOSBase)
{
    BPTR ret = BNULL;

    if (fh)
    {
        BPTR olddir = CurrentDir(fh);
        ret    = Open("", mode);

        CurrentDir(olddir);
    }

    return ret;
}
