/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
        Change the attribute of the given gadget according to the
        attributes chosen in the tag list. If an attribute is not
        provided in the tag list, its value remains unchanged.

    INPUTS
	gad -     Gadget, for which the specified attributes should be set for.
	win -     Window, in which the gadget is.
	req -     Not used. Specify NULL for now.
	tagList - List of attributes to set.

    RESULT
	The gadget may be set to the specified attributes.

    NOTES

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

    if (win || req)
    {
        SetGadgetAttrsA(gad, win, req, tagList);
    } else {
        SetAttrsA((Object *)gad, tagList);
    }
    
    AROS_LIBFUNC_EXIT
    
} /* GT_SetGadgetAttrsA */
