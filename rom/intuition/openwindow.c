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
#include <intuition/intuition.h>

extern struct Window * intui_OpenWindow (struct NewWindow *,
	    struct IntuitionBase *);

/*****************************************************************************

    NAME */
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH1(struct Window *, OpenWindow,

/*  SYNOPSIS */
	__AROS_LHA(struct NewWindow *, newWindow, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 34, Intuition)

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

    return intui_OpenWindow (newWindow, IntuitionBase);

    __AROS_FUNC_EXIT
} /* OpenWindow */
