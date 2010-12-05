/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: openfromlock.c 34705 2010-10-13 20:30:16Z jmcmullan $

    Desc: Open a file from a lock
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

#define DEBUG 0
#include <aros/debug.h>

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


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh;
    struct FileLock *fl = BADDR(lock);
    LONG status = DOSFALSE;

    fh = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);
    if (fh) {
        status = dopacket2(DOSBase, NULL, fl->fl_Task, ACTION_FH_FROM_LOCK, MKBADDR(fh), lock);
        if (!status)
            FreeDosObject(DOS_FILEHANDLE, fh);
    } else {
    	SetIoErr(ERROR_NO_FREE_STORE);
    }
    return status;

    AROS_LIBFUNC_EXIT
} /* OpenFromLock */

