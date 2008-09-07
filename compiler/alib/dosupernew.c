/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG

#include <intuition/classes.h>
#include <utility/tagitem.h>
#include <stdarg.h>
#include <proto/alib.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	IPTR DoSuperNewTagList
        (
/*  SYNOPSIS */
            Class             *CLASS,
            Object            *object,
            struct GadgetInfo *gadgetInfo,
            struct TagItem    *tags
        )

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	boopsi.library/NewObjectA(), boopsi.library/SetAttrsA(), boopsi.library/GetAttr(),
	boopsi.library/DisposeObject(), DoMethodA(),
        CoerceMethodA(), <intuition/classes.h>

******************************************************************************/
{
    if (CLASS == NULL || object == NULL) return NULL;
    
    return DoSuperMethod(CLASS, object, OM_NEW, tags, gadgetInfo);
} /* DoSuperNewTagList() */

IPTR DoSuperNewTags
(
    Class *CLASS,
    Object *object,
    struct GadgetInfo *gadgetInfo,
    Tag tag1,
    ...
)
{
    if (CLASS == NULL || object == NULL)
        return NULL;
        
    AROS_SLOWSTACKMETHODS_PRE(tag1)
    retval = DoSuperNewTagList(CLASS, object, gadgetInfo, (struct TagItem *) AROS_SLOWSTACKMETHODS_ARG(tag1));
    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperNewTags() */
