/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include <unistd.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, Delay,

/*  SYNOPSIS */
	AROS_LHA(ULONG, timeout, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 33, Dos)

/*  FUNCTION
        Waits for at least the time specified as timeout.

    INPUTS
        timeout - the minimum time to wait in ticks (1/50 seconds)

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
