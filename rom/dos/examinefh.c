/*
    Copyright © 1995-2016, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function ExamineFH().
    Lang: English
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, ExamineFH,

/*  SYNOPSIS */
        AROS_LHA(BPTR,                   fh, D1),
        AROS_LHA(struct FileInfoBlock *, fib,  D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 65, Dos)
/*
    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *handle = BADDR(fh);
    BOOL ret;

    D(bug("[ExamineFH] handle=%x fib=%x\n", handle, fib));
    ret = dopacket2(DOSBase, NULL, handle->fh_Type, ACTION_EXAMINE_FH,
        handle->fh_Arg1, MKBADDR(fib));
    if (ret) {
        fixfib(fib);
        D(bug("[ExamineFH] '%s'\n", fib->fib_FileName));
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* ExamineFH */
