/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include "dos_intern.h"

AROS_UFH2(void,vfp_hook,
          AROS_UFHA(UBYTE,        chr, D0),
          AROS_UFHA(struct vfp *, vfp, A3))
{
    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase = vfp->DOSBase;

    if (vfp->count >= 0 && chr != '\0')
    {
        if (FPutC(vfp->file, chr) < 0)
        {
            vfp->count = -1;

            return;
        }

        vfp->count++;
    }

    AROS_USERFUNC_EXIT
}

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, VFPrintf,

/*  SYNOPSIS */
        AROS_LHA(BPTR,         file,     D1),
        AROS_LHA(CONST_STRPTR, format,   D2),
        AROS_LHA(const IPTR *, argarray, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 59, Dos)

/*  FUNCTION
        Write a formatted (RawDoFmt) string to a specified file (buffered).

    INPUTS
        file     - Filehandle to write to
        format   - RawDoFmt() style formatting string
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

    vfp.file = file;
    vfp.count = 0;
    vfp.DOSBase = DOSBase;

    (void)RawDoFmt(format, (APTR)argarray,
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
} /* VFPrintf */
