/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a hard- or softlink.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, MakeLink,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(void *, dest, D2),
	AROS_LHA(LONG  , soft, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 74, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("MakeLink");
    SetIoErr(ERROR_NOT_IMPLEMENTED);

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* MakeLink */
