/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <exec/memory.h>
#include <intuition/cghooks.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <utility/tagitem.h>

AROS_LH4(IPTR, SetGadgetAttrsA,

         /*  SYNOPSIS */
         AROS_LHA(struct Gadget *,    gadget,    A0),
         AROS_LHA(struct Window *,    window,    A1),
         AROS_LHA(struct Requester *, requester, A2),
         AROS_LHA(struct TagItem *,   tagList,   A3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 110, Intuition)

/*  FUNCTION
    Sets some tags and provides gadget specific data. Prefer this to
    SetAttrsA(), if you are manipulating gadgets.
 
    INPUTS
    gadget - Change the attributes of this gadget
    window - The window of the gadget
    requester - The requester of the gadget (or NULL)
    tagList - This is a list of attribute/value-pairs
 
    RESULT
    Depends in the class. For gadgets, this value is non-zero if
    they need redrawing after the values have changed. Other classes
    will define other return values.
 
    NOTES
    This function sends OM_SET to the gadget object.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    NewObject(), SetAttrsA(), GetAttr(), DoGadgetMethodA(),
    "Basic Object-Oriented Programming System for Intuition" and
    "boopsi Class Reference" Dokument.
 
    INTERNALS
    SetGadgetAttrsA(gad, win, req, tags) ist just a replacement for
    DoGadgetMethod(gad, win, req, OM_SET, tags, NULL).
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct opSet ops;
    IPTR    	 result;

    DEBUG_SETGADGETATTRS(dprintf("SetGadgetAttrs[%x]: Gadget 0x%lx Window 0x%lx Requester 0x%lx Tags 0x%lx\n",
                                 &ops, gadget, window, requester, tagList));

    SANITY_CHECKR(gadget,FALSE)
    SANITY_CHECKR(tagList,FALSE)

#if 0
    if (tagList)
    {
        struct TagItem *state = tagList;
        struct TagItem *tag;

        while (tag = NextTagItem(&state))
        {
            dprintf("\t%08lx %08lx\n", tag->ti_Tag, tag->ti_Data);
        }
    }
#endif

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = tagList;
    ops.ops_GInfo    = 0; /* Not really necessary as it will be filled in by DoGadgetMethodA */

    if (window || requester)
    {
        result = DoGadgetMethodA(gadget, window, requester, (Msg)&ops);
    }
    else
    {
        result = DoMethodA((Object *)gadget, (Msg)&ops);
    }

    DEBUG_SETGADGETATTRS(dprintf("SetGadgetAttrs[%x]: Return 0x%lx\n", &ops, result));

    return result;

    AROS_LIBFUNC_EXIT

} /* SetGadgetAttrsA */
