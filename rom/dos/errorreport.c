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

	AROS_LH4(LONG, ErrorReport,

/*  SYNOPSIS */
	AROS_LHA(LONG            , code, D1),
	AROS_LHA(LONG            , type, D2),
	AROS_LHA(ULONG           , arg1, D3),
	AROS_LHA(struct MsgPort *, device, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 80, Dos)

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

#warning TODO: Write dos/ErrorReport()
    aros_print_not_implemented ("ErrorReport");

    return 1;
    AROS_LIBFUNC_EXIT
} /* ErrorReport */
