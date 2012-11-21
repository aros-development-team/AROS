/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Query a filesystem for interactiveness.
    Lang: English
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, IsInteractive,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 36, Dos)

/*  FUNCTION
        Check if file is bound to an interactive device such as a console
        or shell window.

    INPUTS
        file   - filehandle

    RESULT
        != 0 if the file is interactive, 0 if it is not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(file);

    if (fh == NULL)
        return DOSFALSE;

    return (LONG)fh->fh_Interactive; /* 100% identical to official ROM behavior */

    AROS_LIBFUNC_EXIT
} /* IsInteractive */
