/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Change the date of a file.
*/

#include <string.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <dos/dos.h>

        AROS_LH2(BOOL, SetFileDate,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR,       name, D1),
        AROS_LHA(const struct DateStamp *, date, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 66, Dos)

/*  FUNCTION
        Change the modification time of a file or directory.

    INPUTS
        name - name of the file
        date - new file time

    RESULT
        Boolean success indicator. IoErr() gives additional information upon
        failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[SetFileDate] '%s' %x\n", name, date));

    return pathpacket(DOSBase, name, ACTION_SET_DATE, TRUE, (SIPTR)date, 0);

    AROS_LIBFUNC_EXIT
} /* SetFileDate */
