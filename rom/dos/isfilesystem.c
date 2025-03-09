/*
    Copyright (C) 1995-2008, The AROS Development Team. All rights reserved.

    Desc: Check if a device is a filesystem.
*/

#include "dos_intern.h"

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
        devicename      - Name of the device to query.

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

    return IsFileSystemRelative(NULL, devicename);
    
    AROS_LIBFUNC_EXIT
} /* IsFileSystem */
