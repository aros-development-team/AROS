/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:52  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:59  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include "dos_intern.h"

__RA2(void,vfp_hook,UBYTE,chr,D0,struct vfp*,vfp,A3)
{
    __AROS_FUNC_INIT
    extern struct DosLibrary *DOSBase;
    if(vfp->count>=0)
    {
	if(FPUTC(vfp->file,chr)<0)
        {
            vfp->count=-1;
            return;
        }
        vfp->count++;
    }
    __AROS_FUNC_EXIT
}

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH3(LONG, VFPrintf,

/*  SYNOPSIS */
	__AROS_LHA(BPTR,   file,     D1),
	__AROS_LHA(STRPTR, format,   D2),
	__AROS_LHA(LONG *, argarray, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 59, Dos)

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
    
    vfp.file=file;
    vfp.count=0;

    (void)RawDoFmt(format,argarray,(VOID_FUNC)vfp_hook,&vfp);

    /* Remove the last character (which is a NUL character) */
    if(vfp.count>0)
    {
        vfp.count--;
        ((struct FileHandle *)BADDR(file))->fh_Pos--;
    }
    return vfp.count;
    __AROS_FUNC_EXIT
} /* VFPrintf */
