/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.10  2000/03/12 23:34:36  bernie
    Format string made const

    Revision 1.9  1998/10/20 16:44:56  hkiel
    Amiga Research OS

    Revision 1.8  1997/01/27 00:36:33  ldp
    Polish

    Revision 1.7  1996/12/09 13:53:49  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.6  1996/11/18 13:20:14  aros
    Compile assembler files with CPP

    New macros: AROS_ASMSYMNAME() (replaces AROS_ASMFUNC_NAME()) and
    AROS_CSYMNAME() which must be used to access assembler symbols from C and C
    symbols from assembler.

    Revision 1.5  1996/10/24 15:50:38  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/09/13 17:50:09  digulla
    Use IPTR

    Revision 1.3  1996/08/13 13:52:52  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:59  digulla
    Added standard header for all files

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
