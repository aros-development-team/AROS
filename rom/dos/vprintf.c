/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:36:34  ldp
    Polish

    Revision 1.6  1996/12/09 13:53:50  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.5  1996/10/24 15:50:38  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/09/13 17:50:09  digulla
    Use IPTR

    Revision 1.3  1996/08/13 13:52:53  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:59  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dos.h>
#include "dos_intern.h"

void vfp_hook();

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, VPrintf,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, format,   D1),
	AROS_LHA(IPTR *, argarray, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 159, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct vfp vfp;
    BPTR file=((struct Process *)FindTask(NULL))->pr_COS;

    vfp.file=file;
    vfp.count=0;

    (void)RawDoFmt(format,argarray,vfp_hook,&vfp);

    /* Remove the last character (which is a NUL character) */
    if(vfp.count>0)
    {
	vfp.count--;
	((struct FileHandle *)BADDR(file))->fh_Pos--;
    }
    return vfp.count;
    AROS_LIBFUNC_EXIT
} /* VPrintf */
