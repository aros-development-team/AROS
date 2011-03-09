/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level filesystem access functions, IOFS version
    Lang: English
*/

#include <aros/debug.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "fs_driver.h"

BYTE DosDoIO(struct IORequest *iORequest)
{
    return DoIO(iORequest);
} /* DosDoIO */

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, CONST_STRPTR name, LONG accessMode, struct DosLibrary *DOSBase)
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

LONG fs_Open(struct FileHandle *handle, UBYTE refType, BPTR ref, LONG accessMode, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    struct IOFileSys iofs;
    LONG doappend = 0;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_OPEN_FILE, DOSBase);

    switch (accessMode)
    {
    case MODE_OLDFILE:
	iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_OLDFILE;
	break;

    case MODE_NEWFILE:
	iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_NEWFILE;
	break;

    case MODE_READWRITE:
	iofs.io_Union.io_OPEN_FILE.io_FileMode = FMF_MODE_READWRITE;
	break;

    default:
	/* See if the user requested append mode */
	doappend = accessMode & FMF_APPEND;
	/* The append mode is all taken care by dos.library */
	iofs.io_Union.io_OPEN_FILE.io_FileMode = accessMode & ~FMF_APPEND;
	break;
    }

    if (refType == REF_DEVICE)
    {
        struct DevProc *dvp = BADDR(ref);

	iofs.IOFS.io_Device = (struct Device *)dvp->dvp_Port;
	if (dvp->dvp_Lock != BNULL)
	    iofs.IOFS.io_Unit = ((struct FileHandle *)BADDR(dvp->dvp_Lock))->fh_Unit;
	else
            iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;
    }
    else
    {
    	/* In IOFS consoles and regular locks are the same */
    	struct FileHandle *fh = BADDR(ref);

    	iofs.IOFS.io_Device = fh->fh_Device;
	iofs.IOFS.io_Unit   = fh->fh_Unit;
    }

    iofs.io_Union.io_OPEN_FILE.io_Filename = (refType == REF_CONSOLE) ? (STRPTR)"" : name;

    DosDoIO(&iofs.IOFS);

    if (!iofs.io_DosError)
    {
        handle->fh_Device = iofs.IOFS.io_Device;
        handle->fh_Unit   = iofs.IOFS.io_Unit;

        if (doappend)
        {
            /* See if the handler supports FSA_SEEK */
            if (Seek(MKBADDR(handle), 0, OFFSET_END) != -1)
            {
        	/* if so then set the proper flag in the FileHandle struct */
        	handle->fh_Flags |= FHF_APPEND;
            }
        }
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
