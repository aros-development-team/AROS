/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/exall.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(APTR, AllocDosObject,

/*  SYNOPSIS */
	__AROS_LA(ULONG,            type, D1),
	__AROS_LA(struct TagItem *, tags, D2),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
    __AROS_FUNC_EXIT
} /* AllocDosObject */
