/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <exec/types.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>

        AROS_LH3(BOOL, LayoutMenusA,

/*  SYNOPSIS */
	AROS_LHA(struct Menu *, menu, A0),
	AROS_LHA(APTR, vi, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, GadtoolsBase, 11, Gadtools)

/*  FUNCTION

    INPUTS
	menu -     Menu to be layouted.
	vi -       Visual info to layout the menu for.
	tagList  - Additional tags.

    RESULT
	FALSE, if an error occured.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GT_LayoutMenuItemsA()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadtoolsBase *,GadtoolsBase)

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* GT_LayoutMenusA */
