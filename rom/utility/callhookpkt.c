/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/12/27 08:19:34  iaint
    Use UFC macro for registerized params

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

    return AROS_UFC3(IPTR, hook->h_Entry,
	AROS_UFCA(struct Hook *, hook, A0),
	AROS_UFCA(APTR,		 object, A2),
	AROS_UFCA(APTR,	 paramPacket, A1)
    );

    AROS_LIBFUNC_EXIT
} /* CallHookPkt */
