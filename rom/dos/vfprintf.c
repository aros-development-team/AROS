/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/asmcall.h>
#include "dos_intern.h"

AROS_UFH2(void,vfp_hook,
    AROS_UFHA(UBYTE,        chr, D0),
    AROS_UFHA(struct vfp *, vfp, A3)
)
{
    AROS_LIBFUNC_INIT
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
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, VFPrintf,

/*  SYNOPSIS */
	AROS_LHA(BPTR,         file,     D1),
	AROS_LHA(CONST_STRPTR, format,   D2),
	AROS_LHA(IPTR *,       argarray, D3),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    struct vfp vfp;

    vfp.file=file;
    vfp.count=0;

    (void) RawDoFmt (format,
	argarray,
	(VOID_FUNC)AROS_ASMSYMNAME(vfp_hook),
	&vfp
    );

    /* Remove the last character (which is a NUL character) */
    if(vfp.count>0)
    {
	vfp.count--;
	((struct FileHandle *)BADDR(file))->fh_Pos--;
    }
    return vfp.count;
    AROS_LIBFUNC_EXIT
} /* VFPrintf */
