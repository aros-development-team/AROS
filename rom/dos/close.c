/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  2000/11/15 20:16:07  SDuvan
    Simplified and unified

    Revision 1.8  1998/10/20 16:44:29  hkiel
    Amiga Research OS

    Revision 1.7  1997/01/27 00:36:15  ldp
    Polish

    Revision 1.6  1996/12/09 13:53:22  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.5  1996/10/24 15:50:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:52:45  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.3  1996/08/12 14:17:34  digulla
    Added alias UnLock Close

    Revision 1.2  1996/08/01 17:40:48  digulla
    Added standard header for all files

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
/*AROS alias UnLock Close */
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
    DoIO(&iofs.IOFS);

    /* Free the filehandle which was allocated in Open(), CreateDir()
       and such. */
    FreeDosObject(DOS_FILEHANDLE, fh);

    return ret;

    AROS_LIBFUNC_EXIT
} /* Close */
