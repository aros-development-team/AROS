/*
    Copyright � 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file from a lock
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
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
    SIPTR err = -2;

    if (lock == BNULL)
        return BNULL;

    fh = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE, NULL);
    if (fh) {
        struct MsgPort *port = fl->fl_Task;

        if (port == BNULL) {
            /* Special case for NIL: */
            fh->fh_Interactive = DOSTRUE;
            FreeMem(fl, sizeof(*fl));
            err = DOSTRUE;
        } else {
            /* Normal case */
            err = dopacket2(DOSBase, NULL, port, ACTION_FH_FROM_LOCK, MKBADDR(fh), lock);
        }

        if (err != DOSTRUE) {
            FreeDosObject(DOS_FILEHANDLE, fh);
            fh = NULL;
        } else {
            fh->fh_Type = port;
            if (IsInteractive(MKBADDR(fh) && fl->fl_Access == ACCESS_WRITE))
                SetVBuf(MKBADDR(fh), NULL, BUF_LINE, -1);
        }
    } else {
    	SetIoErr(ERROR_NO_FREE_STORE);
    }

    D(bug("[OpenFromLock] %p => fh = %p (%p), error = %d\n", BADDR(lock), fh, fh->fh_Type, err));
    return fh ? MKBADDR(fh) : BNULL;

    AROS_LIBFUNC_EXIT
} /* OpenFromLock */

