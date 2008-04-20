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
	Since locks and filehandles in AROS are identical this function
	is just the (useless) identity operator and thus can never fail.
	It's there for compatibility to Amiga OS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	As of 04/17/08:
	- the above note is not true anymore
	- the AROS packet implementation and the various DOS packets convertion
	  layers can't handle packet type ACTION_FH_FROM_LOCK wich does not
	  exist in AROS as a consequence of the above note
	- we can continue to hack this way until death
	- the current OpenFromLock implementation is hacked to handle SFS
	- by the way, C:Touch does not work on SFS (AROS packets handling bug)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MsgPort *mp;
    struct IOFileSys iofs;
    struct DosPacket pkt = {0};
    struct FileHandle *file;
    struct FileHandle *fh;

    struct ASFSHandle
    {
	void *handle;
	ULONG flags;
	struct ASFSDeviceInfo *device;
    };

    struct ASFSHandle *asfshandle;

    if (!lock)
    {
	SetIoErr(ERROR_INVALID_LOCK);
	return NULL;
    }

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

	file = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);
	if (!file)
	{
	    DeleteMsgPort(mp);
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	file->fh_Pos = file->fh_End = (UBYTE *)-1;

	pkt.dp_Type = ACTION_FH_FROM_LOCK;
	pkt.dp_Arg1 = (SIPTR)MKBADDR(file);
	pkt.dp_Arg2 = (SIPTR)asfshandle->handle;

	iofs.io_PacketEmulation = &pkt;

	DosDoIO((struct IORequest *)&iofs);

        D(bug("OpenFromLock dp_Res1=%d IoErr=%d\n",
	      pkt.dp_Res1, iofs.io_DosError));
	switch (iofs.io_DosError)
	{
	case ERROR_ACTION_NOT_KNOWN:
	case ERROR_NOT_IMPLEMENTED:
	    FreeDosObject(DOS_FILEHANDLE, file);
            break;
	default:
	    if (pkt.dp_Res1)
	    {
		file->fh_Device = iofs.IOFS.io_Device;
		file->fh_Unit   = iofs.IOFS.io_Unit;
		lock = MKBADDR(file);
	    }
	    else
	    {
		SetIoErr(iofs.io_DosError);
		FreeDosObject(DOS_FILEHANDLE, file);
		lock = NULL;
	    }
	}

	DeleteMsgPort(mp);
    }

    return lock;

    AROS_LIBFUNC_EXIT
} /* OpenFromLock */
