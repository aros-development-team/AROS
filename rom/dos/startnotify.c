/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/notify.h>
#include <proto/dos.h>

	AROS_LH1(BOOL, StartNotify,

/*  SYNOPSIS */
	AROS_LHA(struct NotifyRequest *, notify, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 148, Dos)

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

#warning TODO: Write dos/StartNotify()
    aros_print_not_implemented ("StartNotify");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* StartNotify */
