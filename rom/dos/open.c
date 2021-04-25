/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Open a file with the specified mode.
*/

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BPTR, Open,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name,       D1),
        AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 5, Dos)

/*  FUNCTION
        Opens a file for read and/or write depending on the accessmode given.

    INPUTS
        name       - NUL terminated name of the file.
        accessMode - One of MODE_OLDFILE   - open existing file
                            MODE_NEWFILE   - delete old, create new file
                                             exclusive lock
                            MODE_READWRITE - open new one if it doesn't exist

    RESULT
        Handle to the file or 0 if the file couldn't be opened.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return OpenRelative(NULL, name, accessMode);

    AROS_LIBFUNC_EXIT
}
