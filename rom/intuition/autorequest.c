/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/12/10 14:00:00  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:51:17  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:06:48  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>

	AROS_LH8(BOOL, AutoRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Window    *, window, A0),
	AROS_LHA(struct IntuiText *, body, A1),
	AROS_LHA(struct IntuiText *, posText, A2),
	AROS_LHA(struct IntuiText *, negText, A3),
	AROS_LHA(ULONG             , pFlag, D0),
	AROS_LHA(ULONG             , nFlag, D1),
	AROS_LHA(ULONG             , width, D2),
	AROS_LHA(ULONG             , height, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 58, Intuition)

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
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return FALSE; /* TODO */

    AROS_LIBFUNC_EXIT
} /* AutoRequest */
