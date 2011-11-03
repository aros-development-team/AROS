/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve the full pathname from a filehandle.
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, NameFromFH,

/*  SYNOPSIS */
	AROS_LHA(BPTR,   fh    , D1),
	AROS_LHA(STRPTR, buffer, D2),
	AROS_LHA(LONG,   length, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 68, Dos)

/*
    FUNCTION
	Get the full path name associated with file-handle into a
	user supplied buffer.

    INPUTS
	fh     - File-handle to file or directory.
	buffer - Buffer to fill. Contains a NUL terminated string if
		 all went well.
	length - Size of the buffer in bytes.

    RESULT
	!=0 if all went well, 0 in case of an error. IoErr() will
	give additional information in that case.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL res = FALSE;
    struct FileInfoBlock *fib;
    BPTR parentlock = BNULL;
    ULONG err = 0;

    /* DupLock() calls are not allowed because they fail
     * if FH has exclusive lock (MODE_NEWFILE)
     */

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib) {
	parentlock = ParentOfFH(fh);
	if (parentlock) {
	    if (NameFromLock(parentlock, buffer, length)) {
	    	if (ExamineFH(fh, fib)) {
	    	    if (AddPart(buffer, fib->fib_FileName, length)) {
	    	    	res = TRUE;
	    	    }
	    	}
	    }
	}
	err = IoErr(); /* UnLock() clears pr_Result2 */
	FreeDosObject(DOS_FIB, fib);
    } else {
	err = ERROR_NO_FREE_STORE;
    }

    if (parentlock)
	UnLock(parentlock);

    SetIoErr(err);
    return res;

    AROS_LIBFUNC_EXIT
}
