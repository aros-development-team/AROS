/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the size of a file.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, SetFileSize,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file,   D1),
        AROS_LHA(LONG, offset, D2),
        AROS_LHA(LONG, mode,   D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 76, Dos)

/*  FUNCTION
        Change the size of a file.

    INPUTS
        file   - filehandle
        offset - relative size
        mode   - OFFSET_BEGINNING, OFFSET_CURRENT or OFFSET_END

    RESULT
        New size of the file or -1 in case of an error.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = BADDR(file);
    LONG status;

    status = dopacket3(DOSBase, NULL, fh->fh_Type, ACTION_SET_FILE_SIZE, fh->fh_Arg1, offset, mode);

    return status;
   
    AROS_LIBFUNC_EXIT
} /* SetFileSize */
