/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/13 15:37:27  digulla
    First function for intuition.library


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH2(BOOL, ModifyIDCMP,

/*  SYNOPSIS */
	__AROS_LHA(struct Window *, window, A0),
	__AROS_LHA(unsigned long  , flags, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 25, Intuition)

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

    if (!window->IDCMPFlags && flags)
    {
	window->UserPort = CreateMsgPort ();

	if (!window->UserPort)
	    return FALSE;
    }

    window->IDCMPFlags = flags;

    if (!flags)
    {
	if (window->UserPort)
	{
	    DeleteMsgPort (window->UserPort);
	    window->UserPort = NULL;
	}
    }

    return TRUE;
    __AROS_FUNC_EXIT
} /* ModifyIDCMP */
