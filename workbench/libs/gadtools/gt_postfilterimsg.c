/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/intuition.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>

        AROS_LH1(struct IntuiMessage *, GT_PostFilterIMsg,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiMessage *, modimsg, A1),

/*  LOCATION */
	struct Library *, GadtoolsBase, 18, Gadtools)

/*  FUNCTION
	Restores an intuition message formerly changed with GT_FilterIMsg().

    INPUTS
	modimsg - The message returned from GT_FilterIMsg(). May be NULL.

    RESULT
	The original intuition message or NULL, if NULL was passed in.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GT_FilterIMsg()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadtoolsBase *,GadtoolsBase)

    return modimsg;

    AROS_LIBFUNC_EXIT
} /* GT_BeginRefresh */
