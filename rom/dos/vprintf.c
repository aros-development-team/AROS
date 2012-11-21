/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <aros/debug.h>

#include <proto/exec.h>
#include <dos/dos.h>
#include <aros/asmcall.h>
#include "dos_intern.h"

AROS_UFP2(void, vfp_hook,
    AROS_UFPA(UBYTE,        chr, D0),
    AROS_UFPA(struct vfp *, vfp, A3)
);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(LONG, VPrintf,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, format,   D1),
        AROS_LHA(IPTR *,       argarray, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 159, Dos)

/*  FUNCTION
        Writes a formatted string to standard output.

    INPUTS
        format   - RawDoFmt like format string
        argarray - Pointer to array of formatting values

    RESULT
        Number of bytes written or -1 for an error

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct vfp vfp;
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR       file;

    ASSERT_VALID_PROCESS(me);

    file = me->pr_COS;

    vfp.file = file;
    vfp.count = 0;
    vfp.DOSBase = DOSBase;

    (void)RawDoFmt(format, argarray,
                   (VOID_FUNC)AROS_ASMSYMNAME(vfp_hook), &vfp);

    /* Remove the last character (which is a NUL character) */
    /*
    if (vfp.count > 0)
    {
        vfp.count--;
        ((struct FileHandle *)BADDR(file))->fh_Pos--;
    }
    */

    return vfp.count;
    AROS_LIBFUNC_EXIT
} /* VPrintf */
