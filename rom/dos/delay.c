/*
    $Id$
    $Log$
    Revision 1.2  1996/07/29 15:16:42  digulla
    A small hack to make it work until we have timer.device

    Revision 1.1.1.1  1996/07/28 16:37:22  digulla
    First CVS version of AROS

    Desc:
    Lang: english
*/
#include <clib/dos_protos.h>
#include <unistd.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(void, Delay,

/*  SYNOPSIS */
	__AROS_LA(ULONG, timeout, D1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* ADA just to make it work */
    usleep (timeout * 20000L);

    __AROS_FUNC_EXIT
} /* Delay */
