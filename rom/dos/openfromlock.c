/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file from a lock
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include "dos_intern.h"

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
            fh->fh_Interactive = DOSFALSE;
            FreeMem(fl, sizeof(*fl));
            err = DOSTRUE;
        } else {
            /* Normal case */
            err = dopacket2(DOSBase, NULL, port, ACTION_FH_FROM_LOCK, MKBADDR(fh), lock);
        }

        if (err == DOSFALSE) {
            FreeDosObject(DOS_FILEHANDLE, fh);
            fh = NULL;
        } else {
            fh->fh_Type = port;
            /* Buffering for interactive filehandes defaults to BUF_LINE */
            if (fh->fh_Interactive)
                SetVBuf(MKBADDR(fh), NULL, BUF_LINE, -1);
            else
                SetVBuf(MKBADDR(fh), NULL, BUF_NONE, -1);
        }
    } else {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    D(bug("[OpenFromLock] %p => fh = %p (%p), error = %d\n", BADDR(lock), fh, fh->fh_Type, err));
    return fh ? MKBADDR(fh) : BNULL;

    AROS_LIBFUNC_EXIT
} /* OpenFromLock */

