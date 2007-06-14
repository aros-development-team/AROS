/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if a device is a filesystem.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/utility.h>
#include "dos_intern.h"
#include <string.h>

# define  DEBUG 0
# include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, IsFileSystem,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, devicename, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 118, Dos)

/*  FUNCTION
	Query the device whether it is a filesystem.

    INPUTS
	devicename	- Name of the device to query.

    RESULT
	TRUE if the device is a filesystem, FALSE otherwise.

    NOTES
	DF0:, HD0:, ... are filesystems.
	CON:, PIPE:, AUX:, ... are not

        In AmigaOS if devicename contains no ":" then result
	is always TRUE. Also volume and assign names return
	TRUE.
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IOFileSys iofs;
    LONG err;

    /* console is never a filesystem */
    if (Stricmp(devicename, "CONSOLE:") == 0 || Stricmp(devicename, "*") == 0)
        return FALSE;

    InitIOFS(&iofs, FSA_IS_FILESYSTEM, DOSBase);
    err = DoIOFS(&iofs, NULL, devicename, DOSBase);

    /* XXX if err is ERROR_ACTION_NOT_KNOWN, we should try to the lock the
     * root. if we can get a lock, then its a filesystem */

    if (err != 0)
        return FALSE;

    return iofs.io_Union.io_IS_FILESYSTEM.io_IsFilesystem;
    
    AROS_LIBFUNC_EXIT
} /* IsFilesystem */
