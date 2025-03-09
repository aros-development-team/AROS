/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: Rename a file
*/

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

        AROS_LH2(LONG, Rename,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, oldName, D1),
        AROS_LHA(CONST_STRPTR, newName, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 13, Dos)

/*  FUNCTION
        Renames a given file. The old name and the new name must point to the
        same volume.

    INPUTS
        oldName - Name of the file to rename
        newName - New name of the file to rename

    RESULT
        boolean - DOSTRUE or DOSFALSE. IoErr() provides additional information
        on DOSFALSE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return RenameRelative(BNULL, oldName, BNULL, newName);

    AROS_LIBFUNC_EXIT
} /* Rename */
