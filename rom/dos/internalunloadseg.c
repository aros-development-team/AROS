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

	AROS_LH2(BOOL, InternalUnLoadSeg,

/*  SYNOPSIS */
	AROS_LHA(BPTR     , seglist, D1),
	AROS_LHA(VOID_FUNC, freefunc, A1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 127, Dos)

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

    aros_print_not_implemented ("InternalUnLoadSeg");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* InternalUnLoadSeg */
