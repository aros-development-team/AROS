/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/utility.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

    AROS_LH2(ULONG, SetAttrsA,

/*  SYNOPSIS */
        AROS_LHA(APTR            , object, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 108, Intuition)

/*  FUNCTION
        Changes several attributes of an object at the same time. How the
        object interprets the new attributes depends on the class.

    INPUTS
        object - Change the attributes of this object
        tagList - This is a list of attribute/value-pairs

    RESULT
        Depends on the class. For gadgets, this value is non-zero if
        they need redrawing after the values have changed. Other classes
        will define other return values.

    NOTES
        This function sends OM_SET to the object.

    EXAMPLE

    BUGS

    SEE ALSO
        NewObjectA(), DisposeObject(), GetAttr(), MakeClass(),
        "Basic Object-Oriented Programming System for Intuition" and
        "boopsi Class Reference" Document.

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct opSet ops;
    ULONG   	 result;

    DEBUG_SETATTRS(dprintf("SetAttrs[0x%p]: Object 0x%p Tags 0x%p\n", &ops, object, tagList));

    SANITY_CHECKR(object,FALSE)
    SANITY_CHECKR(tagList,FALSE)

#if DEBUG > 1
    if (tagList)
    {
        struct TagItem *state = tagList;
        struct TagItem *tag;

        while (tag = NextTagItem(&state))
        {
            dprintf("\t0x%p 0x%p\n", tag->ti_Tag, tag->ti_Data);
        }
    }
#endif

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = tagList;
    ops.ops_GInfo    = NULL;

    result = DoMethodA (object, (Msg)&ops);

    DEBUG_SETATTRS(dprintf("SetAttrs[0x%p]: Return 0x%08X\n", &ops, result));

    return result;

    AROS_LIBFUNC_EXIT
} /* SetAttrsA() */
