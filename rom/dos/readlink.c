/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH5(LONG, ReadLink,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, port, D1),
	AROS_LHA(BPTR            , lock, D2),
	AROS_LHA(STRPTR          , path, D3),
	AROS_LHA(STRPTR          , buffer, D4),
	AROS_LHA(ULONG           , size, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 73, Dos)

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

    aros_print_not_implemented ("ReadLink");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* ReadLink */
