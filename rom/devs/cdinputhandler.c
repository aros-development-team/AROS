/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: console.device function CDInputHandler()
    Lang: english
*/

/*****************************************************************************

    NAME */
	#include <exec/libraries.h>
	#include <devices/inputevent.h>
	#include <clib/console_protos.h>

	AROS_LH2(struct InputEvent *, CDInputHandler,

/*  SYNOPSIS */
	AROS_LHA(struct InputEvent *, events, A0),
	AROS_LHA(struct Library    *, consoleDevice, A1),

/*  LOCATION */
	struct Library *, ConsoleDevice, 7, Console)

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
			    console_lib.fd and clib/console_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,ConsoleDevice)

    return NULL;

    AROS_LIBFUNC_EXIT
} /* CDInputHandler */
