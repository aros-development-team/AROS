/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

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
