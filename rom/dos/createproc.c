/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(struct MsgPort *, CreateProc,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(LONG  , pri, D2),
	AROS_LHA(BPTR  , segList, D3),
	AROS_LHA(LONG  , stackSize, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 23, Dos)

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

#warning TODO: Write dos/CreateProc()
    aros_print_not_implemented ("CreateProc");

    return NULL;
    AROS_LIBFUNC_EXIT
} /* CreateProc */
