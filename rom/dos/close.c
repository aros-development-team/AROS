/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, Close,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 6, Dos)

/*  FUNCTION
	Close a filehandle opened with Open(). If the file was used
	with buffered I/O the final write may fail and thus Close()
	return an error. The file is closed in any case.

    INPUTS
	file  --  filehandle

    RESULT
	0 if there was an error. != 0 on success.

    NOTES
	This function is identical to UnLock().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/

/*****************************************************************************

    NAME
#include <clib/dos_protos.h>

	AROS_LH1(BOOL, UnLock,

    SYNOPSIS
	AROS_LHA(BPTR, lock, D1),

    LOCATION
	struct DosLibrary *, DOSBase, 15, Dos)

    FUNCTION
	Free a lock created with Lock().

    INPUTS
	lock -- The lock to free

    RESULT

    NOTES
	This function is identical to Close() - see there.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
/*AROS alias UnLock Close */
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(file);

    /* Get space for I/O request. Use stack for now. */
    struct IOFileSys iofs;

    /* The returncode defaults to OK. */
    BOOL ret = 1;

    /* 0 handles are OK */
    if(file == NULL)
	return ret;

    /* If the filehandle has a pending write on it Flush() the buffer. */
    if(fh->fh_Flags & FHF_WRITE)
	ret = Flush(file);

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_CLOSE, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit	= fh->fh_Unit;

    /* Send the request. No errors possible. */
    DosDoIO(&iofs.IOFS);

    /* Free the filehandle which was allocated in Open(), CreateDir()
       and such. */
    FreeDosObject(DOS_FILEHANDLE, fh);

    return ret;

    AROS_LIBFUNC_EXIT
} /* Close */
