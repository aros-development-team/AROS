/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:59  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include "dos_intern.h"

void vfp_hook();

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(LONG, VPrintf,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, format,   D1),
	__AROS_LA(LONG *, argarray, D2),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
    __AROS_FUNC_EXIT
} /* VPrintf */
