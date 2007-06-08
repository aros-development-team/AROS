/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new directory.
    Lang: English
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, CreateDir,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 20, Dos)

/*  FUNCTION
	Creates a new directory under the given name. If all went an
	exclusive lock on the new diretory is returned.

    INPUTS
	name  -- NUL terminated name.

    RESULT
	Exclusive lock to the new directory or 0 if couldn't be created.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh;
    struct IOFileSys iofs;
    struct DevProc *dvp;

    if ((fh = (struct FileHandle *) AllocDosObject(DOS_FILEHANDLE, NULL)) == NULL) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return NULL;
    }

    InitIOFS(&iofs, FSA_CREATE_DIR, DOSBase);
    iofs.io_Union.io_CREATE_DIR.io_Protection = 0;

    if (DoIOFS(&iofs, NULL, name, DOSBase) != 0) {
        FreeDosObject(DOS_FILEHANDLE, fh);
        return NULL;
    }

    fh->fh_Device = iofs.IOFS.io_Device;
    fh->fh_Unit   = iofs.IOFS.io_Unit;

    return MKBADDR(fh);

    AROS_LIBFUNC_EXIT
} /* CreateDir */

