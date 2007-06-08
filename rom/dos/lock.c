/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Locks a file or directory.
    Lang: English
*/

#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

#define  DEBUG  0
#include <aros/debug.h>


/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BPTR, Lock,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,       D1),
	AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 14, Dos)

/*  FUNCTION
	Gets a lock on a file or directory. There may be more than one
	shared lock on a file but only one if it is an exclusive one.
	Locked files or directories may not be deleted.

    INPUTS
	name	   - NUL terminated name of the file or directory.
	accessMode - One of SHARED_LOCK
			    EXCLUSIVE_LOCK

    RESULT
	Handle to the file or directory or 0 if the object couldn't be locked.
	IoErr() gives additional information in that case.

    NOTES
	The lock structure returned by this function is different
	from that of AmigaOS (in fact it is identical to a filehandle).
	Do not try to read any internal fields.

*****************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct DevProc *dvp;
    LONG error;

    ASSERT_VALID_PTR(name);

    /* Sanity check */
    if (name == NULL)
        return NULL;
    
    /* Get pointer to process structure */
        struct Process *
    me = (struct Process *)FindTask(NULL);

    /* Create filehandle */
        struct FileHandle *
    ret = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);

    if (ret != NULL)
    {
    	/* Get pointer to I/O request. Use stackspace for now. */
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
        	    iofs.io_Union.io_OPEN.io_FileMode = accessMode;
        	    break;
    	}

        /* catch the zero-length special case. we can't (currently) call
         * GetDeviceProc(), as it will call NameFromLock(), which will
         * DupLock(), which will end up here */
        if (*name == '\0') {
            struct FileHandle *fh;

            if (me->pr_CurrentDir != NULL)
                fh = BADDR(me->pr_CurrentDir);
            else
                fh = BADDR(DOSBase->dl_SYSLock);

            iofs.io_Union.io_OPEN.io_Filename = "";

            iofs.IOFS.io_Device = fh->fh_Device;
            iofs.IOFS.io_Unit = fh->fh_Unit;

            DosDoIO(&iofs.IOFS);

            error = me->pr_Result2 = iofs.io_DosError;
        }

        else {
            iofs.io_Union.io_OPEN.io_Filename = StripVolume(name);

            dvp = NULL;
    
            do {
                if ((dvp = GetDeviceProc(name, dvp)) == NULL) {
                    error = IoErr();
                    break;
                }

                error = DoIOFS(&iofs, dvp, NULL, DOSBase);
            } while (error == ERROR_OBJECT_NOT_FOUND);

            if (error == ERROR_NO_MORE_ENTRIES)
                error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

            FreeDeviceProc(dvp);
        }
    
    	if (!error)
    	{
    	    ret->fh_Device = iofs.IOFS.io_Device;
    	    ret->fh_Unit   = iofs.IOFS.io_Unit;
    
    	    return MKBADDR(ret);
    	}
        else
        {
            FreeDosObject(DOS_FILEHANDLE, ret);
        }
    }
    else
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* Lock */
