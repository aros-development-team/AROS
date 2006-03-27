/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, ExNext,

/*  SYNOPSIS */
	AROS_LHA(BPTR                  , lock, D1),
	AROS_LHA(struct FileInfoBlock *, fileInfoBlock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 18, Dos)

/*  FUNCTION

    Examine the next entry in a directory.
        
    INPUTS

    lock  --  lock on the direcory the contents of which to examine
    fib   --  a FileInfoBlock previously initialized by Examine()
              (or used before with ExNext())

    RESULT

    success  --  a boolean telling whether the operation was successful
                 or not. A failure occurs also if there is no "next" entry in
		 the directory. Then IoErr() equals ERROR_NO_MORE_ENTRIES.

    NOTES

    If scanning a filesystem tree recursively, you'll need to allocated a
    new FilInfoBlock for each directory level.

    EXAMPLE

    To examine a directory, do the following:

    1.  Pass a lock on the directory and a FileInfoBlock (allocated by
        AllocDosObject()) to Examine().
    2.  Pass the same parameters to ExNext().
    3.  Do something with the FileInfoBlock returned.
    4.  Call ExNext() repeatedly until it returns FALSE and use the
        information you are provided. When ExNext returns FALSE, check IoErr()
	to make sure that there was no real failure (ERROR_NO_MORE_ENTRIES).

    BUGS
        At the moment it is necessary that the lock passed to this
        function is a directory(!!) that has previously been 
        assigned (i.e. assign env: env).

    SEE ALSO

    Examine(), IoErr(), AllocDosObject(), ExAll()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to filehandle */
    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_EXAMINE_NEXT, DOSBase);

    iofs.IOFS.io_Device = fh->fh_Device;
    iofs.IOFS.io_Unit   = fh->fh_Unit;

    iofs.io_Union.io_EXAMINE_NEXT.io_fib = fileInfoBlock;

    /* Send the request. */
    DosDoIO(&iofs.IOFS);

    /* Set error code and return */
    SetIoErr(iofs.io_DosError);

    if(iofs.io_DosError != 0)
	return DOSFALSE;
    else
	return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* ExNext */
