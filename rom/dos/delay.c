/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:52:53  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.3  1996/08/01 17:40:49  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/dos_protos.h>
#include <unistd.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH1(void, Delay,

/*  SYNOPSIS */
	AROS_LHA(ULONG, timeout, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 33, Dos)

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* ADA just to make it work */
    usleep (timeout * 20000L);

    AROS_LIBFUNC_EXIT
} /* Delay */
