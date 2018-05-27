/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get an attribute of an object.
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"
#include <aros/debug.h>
#include <oop/static_mid.h>

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(IPTR, OOP_GetAttr,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object		*, object, A0),
	AROS_LHA(OOP_AttrID         , attrID, A1),
	AROS_LHA(IPTR		*, storage, A2),

/*  LOCATION */
	struct Library *, OOPBase, 16, OOP)

/*  FUNCTION
	Gets the specifed attribute from the object,
	and puts it into storage.

    INPUTS
    	object	- pointer to object from which we want to
	          get an attribute.
		  
	attrID  - Attribute ID for property to get.
	
	storage - Pointer to IPTR the fetched data should be put into.

    RESULT
    	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OOP_SetAttrs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct pRoot_Get p;
    struct IntOOPBase *iOOPBase = (struct IntOOPBase *)OOPBase;
    
    EnterFunc(bug("OOP_GetAttr())\n"));
    
    if (!iOOPBase->ob_mRoot_Get)
        iOOPBase->ob_mRoot_Get = OOP_GetMethodID(IID_Root, moRoot_Get);
	
	
    p.mID	= iOOPBase->ob_mRoot_Get;
    p.attrID	= attrID;
    p.storage	= storage;
    
    /* Call the Get() method on the object */
    
    ReturnPtr ("OOP_GetAttr", IPTR, OOP_DoMethod(object, (OOP_Msg)&p));
    
    AROS_LIBFUNC_EXIT
} /* OOP_GetAttr */
