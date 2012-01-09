/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Check if a device is a filesystem.
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/utility.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, IsFileSystem,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, devicename, D1),

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

    LONG err = ERROR_OBJECT_NOT_FOUND;
    LONG code = DOSFALSE;
    struct DevProc *dvp = NULL;

    /* console is never a filesystem */
    if (Stricmp(devicename, "CONSOLE:") == 0 || Stricmp(devicename, "*") == 0 ||
    	Stricmp(devicename, "CON:") == 0 || Stricmp(devicename, "RAW:") == 0) {
    	SetIoErr(err);
        return code;
    }

    if ((dvp = GetDeviceProc(devicename, dvp))) {
    	if (dvp->dvp_Port != NULL) // NIL: isn't a filesystem
    	    code = dopacket0(DOSBase, NULL, dvp->dvp_Port, ACTION_IS_FILESYSTEM);
    	FreeDeviceProc(dvp);
    } else {
    	SetIoErr(err);
    }
   
    return code;
    
    AROS_LIBFUNC_EXIT
} /* IsFilesystem */
