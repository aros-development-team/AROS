/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/12 14:29:29  digulla
    Change owner of a file

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH2(BOOL, SetOwner,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, name, D1),
	__AROS_LA(long  , owner_info, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 166, Dos)

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

    return FALSE;
    __AROS_FUNC_EXIT
} /* SetOwner */
