/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
	#include <utility/hooks.h>
	#include <clib/utility_protos.h>

	AROS_LH3(ULONG, CallHookPkt,

/*  SYNOPSIS */
	AROS_LHA(struct Hook *, hook, A0),
	AROS_LHA(APTR         , object, A2),
	AROS_LHA(APTR         , paramPacket, A1),

/*  LOCATION */
	struct Library *, UtilityBase, 17, Utility)

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
			    utility_lib.fd and clib/utility_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,UtilityBase)

    return ((*(hook->h_Entry)) (hook, object, paramPacket));

    AROS_LIBFUNC_EXIT
} /* CallHookPkt */
