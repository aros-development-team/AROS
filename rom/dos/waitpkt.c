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

	AROS_LH0(struct DosPacket *, WaitPkt,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 42, Dos)

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

#warning TODO: Write WaitPkt()
    aros_print_not_implemented ("WaitPkt");

    return NULL;
    AROS_LIBFUNC_EXIT
} /* WaitPkt */
