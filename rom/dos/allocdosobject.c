/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <utility/tagitem.h>
#include <dos/rdargs.h>
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
    case DOS_RDARGS:
	return AllocVec(sizeof(struct RDArgs),MEMF_CLEAR);
    }
    return NULL;
    AROS_LIBFUNC_EXIT
} /* AllocDosObject */
