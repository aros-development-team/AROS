/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/alib.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

AROS_LH3(ULONG, GetAttr,

         /*  SYNOPSIS */
         AROS_LHA(ULONG   , attrID, D0),
         AROS_LHA(Object *, object, A0),
         AROS_LHA(IPTR *  , storagePtr, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 109, Intuition)

/*  FUNCTION
    Asks the specified object for the value of an attribute. This is not
    possible for all attributes of an object. Read the documentation for
    the class to find out which can be read and which can't.
 
    INPUTS
    attrID - ID of the attribute you want
    object - Ask the attribute from this object
    storagePtr - This is a pointer to memory which is large enough
        to hold a copy of the attribute. Most classes will simply
        put a copy of the value stored in the object here but this
        behaviour is class specific. Therefore read the instructions
        in the class description carefully.
 
    RESULT
    Mostly TRUE if the method is supported for the specified attribute
    and FALSE if it isn't or the attribute can't be read at this time.
    See the classes documentation for details.
 
    NOTES
    This function sends OM_GET to the object.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    NewObject(), DisposeObject(), SetAttr(), MakeClass(),
    "Basic Object-Oriented Programming System for Intuition" and
    "boopsi Class Reference" Dokument.
 
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct opGet get;
    ULONG        result = 0;

    DEBUG_GETATTR(dprintf("GetAttr[%x]: AttrID 0x%lx Object 0x%lx Storage 0x%lx\n",
                          &get, attrID, object, storagePtr));


    SANITY_CHECKR(object,0)
    SANITY_CHECKR(storagePtr,0)

    get.MethodID    = OM_GET;
    get.opg_AttrID  = attrID;
    get.opg_Storage = storagePtr;

    result = DoMethodA (object, (Msg)&get);

    DEBUG_GETATTR(dprintf("GetAttr[%x]: Return %d\n", &get, result));

    return result;

    AROS_LIBFUNC_EXIT
} /* GetAttr() */
