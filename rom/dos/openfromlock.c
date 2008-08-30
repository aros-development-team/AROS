/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file from a lock
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#ifndef SFS_SPECIFIC_MESSAGE
#define SFS_SPECIFIC_MESSAGE 0xff00
#endif

static BPTR SFSOpenFromLock(BPTR lock, struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, OpenFromLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 63, Dos)

/*  FUNCTION
	Convert a lock into a filehandle. If all went well the lock
	will be gone. In case of an error it must still be freed.

    INPUTS
	lock - Lock to convert.

    RESULT
	New filehandle or 0 in case of an error. IoErr() will give
	additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Since locks and filehandles in AROS are identical this function
	is just the (useless) identity operator and thus can never fail.
	It's there for compatibility with Amiga OS.

	As of 04/17/08:
	- the above note is not true anymore
	- the AROS packet implementation and the various DOS packets conversion
	  layers can't handle packet type ACTION_FH_FROM_LOCK which does not
	  exist in AROS as a consequence of the above note
	- we can continue to hack this way until death
	- the current OpenFromLock implementation is hacked to handle SFS
	- by the way, C:Touch does not work on SFS (AROS packets handling bug)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR file = NULL, test_lock, old_dir;
    ULONG mode;

    if (lock == NULL)
    {
	SetIoErr(ERROR_INVALID_LOCK);
	return NULL;
    }

    /* Try SFS hack first */
    file = SFSOpenFromLock(lock, DOSBase);

    /* If SFS hack failed, try more general method that should work for both
     * device-based and packet-based handlers */
    if (file == NULL)
    {
        /* Get lock's mode */
        test_lock = DupLock(lock);
        if (test_lock != (BPTR)NULL)
        {
            mode = SHARED_LOCK;
            UnLock(test_lock);
        }
        else
            mode = EXCLUSIVE_LOCK;

        /* If necessary, switch to a SHARED_LOCK temporarily so we can open
           the file */
        if (mode == EXCLUSIVE_LOCK)
            ChangeMode(CHANGE_LOCK, lock, SHARED_LOCK);

        /* Open file and dispose of no longer needed lock */
        old_dir = CurrentDir(lock);
        file = Open("", MODE_OLDFILE);
        CurrentDir(old_dir);
        if (file != (BPTR)NULL)
            UnLock(lock);

        /* Restore old mode */
        if (mode == EXCLUSIVE_LOCK)
        {
            if (file != (BPTR)NULL)
                ChangeMode(CHANGE_FH, file, mode);
            else
                ChangeMode(CHANGE_LOCK, lock, mode);
        }
    }

    return file;

    AROS_LIBFUNC_EXIT
} /* OpenFromLock */

static BPTR SFSOpenFromLock(BPTR lock, struct DosLibrary *DOSBase)
{
    struct MsgPort *mp;
    struct IOFileSys iofs;
    struct DosPacket pkt = {0};
    struct FileHandle *fh;
    BPTR file = NULL;

    struct ASFSHandle
    {
	void *handle;
	ULONG flags;
	struct ASFSDeviceInfo *device;
    };

    struct ASFSHandle *asfshandle;

    fh = (struct FileHandle *)BADDR(lock);

    if (fh->fh_Unit)
    {
	asfshandle = (struct ASFSHandle *)fh->fh_Unit;

	mp = CreateMsgPort();
	if (!mp)
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	InitIOFS(&iofs, SFS_SPECIFIC_MESSAGE, DOSBase);

	iofs.IOFS.io_Device  = fh->fh_Device;
	iofs.IOFS.io_Unit    = fh->fh_Unit;

	fh = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);
	if (!fh)
	{
	    DeleteMsgPort(mp);
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	fh->fh_Pos = fh->fh_End = (UBYTE *)-1;

	pkt.dp_Type = ACTION_FH_FROM_LOCK;
	pkt.dp_Arg1 = (SIPTR)MKBADDR(fh);
	pkt.dp_Arg2 = (SIPTR)asfshandle->handle;

	iofs.io_PacketEmulation = &pkt;

	DosDoIO((struct IORequest *)&iofs);

        D(bug("OpenFromLock dp_Res1=%d IoErr=%d\n",
	      pkt.dp_Res1, iofs.io_DosError));
	switch (iofs.io_DosError)
	{
	case ERROR_ACTION_NOT_KNOWN:
	case ERROR_NOT_IMPLEMENTED:
	    FreeDosObject(DOS_FILEHANDLE, fh);
            file = NULL;
            break;
	default:
	    if (pkt.dp_Res1)
	    {
		fh->fh_Device = iofs.IOFS.io_Device;
		fh->fh_Unit   = iofs.IOFS.io_Unit;
		file = MKBADDR(fh);
	    }
	    else
	    {
		SetIoErr(iofs.io_DosError);
		FreeDosObject(DOS_FILEHANDLE, fh);
		file = NULL;
	    }
	}

	DeleteMsgPort(mp);
    }

    return file;
}

