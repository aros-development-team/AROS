/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(BOOL, Fault,

/*  SYNOPSIS */
	AROS_LHA(LONG  , code, D1),
	AROS_LHA(STRPTR, header, D2),
	AROS_LHA(STRPTR, buffer, D3),
	AROS_LHA(LONG  , len, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 78, Dos)

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

    aros_print_not_implemented ("Fault");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* Fault */
