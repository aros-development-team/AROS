/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/10/21 17:06:48  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

extern void intui_EndRefresh (struct Window * window,
	    BOOL complete,
	    struct IntuitionBase * IntuitionBase);

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH2(void, EndRefresh,

/*  SYNOPSIS */
	__AROS_LHA(struct Window *, window, A0),
	__AROS_LHA(BOOL           , complete, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 61, Intuition)

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

    intui_EndRefresh (window, complete, IntuitionBase);

    __AROS_FUNC_EXIT
} /* EndRefresh */
