/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <intuition/classes.h>
#include <proto/alib.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

AROS_LH1(void, DisposeObject,

         /*  SYNOPSIS */
         AROS_LHA(APTR, object, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 107, Intuition)

/*  FUNCTION
    Deletes a BOOPSI object. All memory associated with the object
    is freed. The object must have been created with NewObject().
    Some object contain other object which might be freed as well
    when this function is used on the "parent" while others might
    also contain children but won't free them. Read the documentation
    of the class carefully to find out how it behaves.
 
    INPUTS
    object - The result of a call to NewObject() or a similar function,
         may be NULL.
 
    RESULT
    None.
 
    NOTES
    This functions sends OM_DISPOSE to the oejct.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    NewObject(), SetAttrs((), GetAttr(), MakeClass()
    "Basic Object-Oriented Programming System for Intuition" and
    "boopsi Class Reference" Dokument.
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    STACKULONG MethodID = OM_DISPOSE;

    DEBUG_DISPOSEOBJECT(dprintf("DisposeObject: Object 0x%lx\n", object));

    if (!object)
        return;

    DoMethodA (object, (Msg)&MethodID);

    DEBUG_DISPOSEOBJECT(dprintf("DisposeObject: 0x%lx deleted\n", object));

    DEBUG_DISPOSEOBJECT(dprintf("DisposeObject: done\n"));
    
    AROS_LIBFUNC_EXIT
} /* DisposeObject() */
