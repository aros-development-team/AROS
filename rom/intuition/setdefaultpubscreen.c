/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/21 15:48:48  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH1(void, SetDefaultPubScreen,

/*  SYNOPSIS */
	__AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 90, Intuition)

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
/*
    struct PubScreenNode * pscreen;

    if ((pscreen = (struct PubScreenNode *)FindName (
	    &(GetPrivIBase(IntuitionBase)->PublicScreenList)
	)
    ))
    {
	GetPrivIBase(IntuitionBase)->DefaultPublicScreen =
	    pscreen->psn_Screen;
    }
*/

    __AROS_FUNC_EXIT
} /* SetDefaultPubScreen */
