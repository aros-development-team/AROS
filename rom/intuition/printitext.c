/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, PrintIText,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort  *, rp, A0),
	__AROS_LHA(struct IntuiText *, iText, A1),
	__AROS_LHA(long              , left, D0),
	__AROS_LHA(long              , top, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 36, Intuition)

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    __AROS_FUNC_EXIT
} /* PrintIText */
