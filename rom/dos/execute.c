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

	AROS_LH3(LONG, Execute,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, string, D1),
	AROS_LHA(BPTR  , file, D2),
	AROS_LHA(BPTR  , file2, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 37, Dos)

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

#warning TODO: Write dos/Execute()
    aros_print_not_implemented ("Execute");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* Execute */
