/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/classes.h>
#include <stdarg.h>
#include <proto/alib.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	IPTR DoSuperNewA
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
	NewObjectA(), SetAttrsA(), GetAttr(), DisposeObject(), DoMethodA(),
        CoerceMethodA(), <intuition/classes.h>

******************************************************************************/
{
    if (CLASS == NULL || object == NULL) return NULL;
    
    return DoSuperMethod(CLASS, object, OM_NEW, tags, gadgetInfo);
} /* DoSuperNewA */

ULONG DoSuperNewTags
(
    Class *CLASS, Object *object, struct GadgetInfo *gadgetInfo, 
    ULONG methodID, ...
)
{
    AROS_SLOWSTACKMETHODS_PRE(methodID)
    
    if (CLASS == NULL || object == NULL)
    {
        retval = NULL;
    }
    else
    {
        retval = DoSuperNewA
        (
            CLASS, object, gadgetInfo, AROS_SLOWSTACKMETHODS_ARG(methodID)
        );
    }

    AROS_SLOWSTACKMETHODS_POST
} /* DoSuperNewTags */

