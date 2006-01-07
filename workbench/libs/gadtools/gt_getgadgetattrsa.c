/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <exec/ports.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <exec/types.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>

        AROS_LH4(LONG, GT_GetGadgetAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget *, gad, A0),
	AROS_LHA(struct Window *, win, A1),
	AROS_LHA(struct Requester *, req, A2),
	AROS_LHA(struct TagItem *, taglist, A3),

/*  LOCATION */
	struct Library *, GadToolsBase, 29, GadTools)

/*  FUNCTION
        Get a list of attributes from a specific gadget.

    INPUTS
        gad -     the gadget from which to get attributes.
	    	  may be null. if so, this function returns 0.
	win -     the window, in which the gadget is
	req -     the requester, in which the gadget is, or NULL
	taglist - the list of attributes to get. ti_Tag specifies the attribute
	          and ti_Data is a pointer to an IPTR, where the data is to be
	          stored

    RESULT
	The number of attributes, which were filled correctly.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GT_SetGadgetAttrsA(), intuition.library/GetAttr()

    INTERNALS

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)
    
    LONG                  count = 0;
    struct TagItem 	     *tag;
    const struct TagItem *mytags = taglist;

    if (gad == NULL || taglist == NULL)
        return 0L;

    while ((tag = NextTagItem(&mytags)))
        if (GetAttr(tag->ti_Tag, (Object *)gad, (IPTR *)tag->ti_Data))
	    count++;

    return count;
    
    AROS_LIBFUNC_EXIT
    
} /* GT_GetGadgetAttrsA */
