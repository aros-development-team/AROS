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
	#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>

        AROS_LH4(void, GT_SetGadgetAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget *, gad, A0),
	AROS_LHA(struct Window *, win, A1),
	AROS_LHA(struct Requester *, req, A2),
	AROS_LHA(struct TagItem *, tagList, A3),

/*  LOCATION */
	struct Library *, GadToolsBase, 7, GadTools)

/*  FUNCTION
        Refreshes the given window. This function must be used instead
        of BeginRefresh(), if gadtools-gadgets are used. When you are
        finished with refreshing the window, you must call GT_EndRefresh().

    INPUTS
	gad -     Gadget, for which the specified attributes should be set for.
	win -     Window, in which the gadget is.
	req -     Not used. Specify NULL for now.
	tagList - List of attributes to set.

    RESULT
	The gadget may be set to the specified attributes.

    NOTES
	Note that there is no possibility to check, if the setting of an
	attribute was successful, except by getting the attribute and
	checking it. The setting of some attributes cannot fail. Refer to
	the description of the tags to check this.

    EXAMPLE

    BUGS

    SEE ALSO
	GT_GetGadgetAttrsA(), intuition.library/SetGadgetAttrsA()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    SetGadgetAttrsA(gad, win, req, tagList);

    AROS_LIBFUNC_EXIT
} /* GT_BeginRefresh */
