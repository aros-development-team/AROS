/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1997/01/27 00:36:14  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:20  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:44  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:47  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <utility/tagitem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(APTR, AllocDosObject,

/*  SYNOPSIS */
	AROS_LHA(ULONG,            type, D1),
	AROS_LHA(struct TagItem *, tags, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 38, Dos)

/*  FUNCTION
	Creates a new dos object of a given type.

    INPUTS
	type - object type.
	tags - Pointer to taglist array with additional information.

    RESULT
	Pointer to new object or NULL.

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

    switch(type)
    {
	case DOS_FILEHANDLE:
	    return AllocMem(sizeof(struct FileHandle),MEMF_CLEAR);
	case DOS_FIB:
	    return AllocMem(sizeof(struct FileInfoBlock),MEMF_CLEAR);
	case DOS_EXALLCONTROL:
	    return AllocMem(sizeof(struct ExAllControl),MEMF_CLEAR);
	case DOS_CLI:
	    return AllocMem(sizeof(struct CommandLineInterface),MEMF_CLEAR);
    }
    return NULL;
    AROS_LIBFUNC_EXIT
} /* AllocDosObject */
