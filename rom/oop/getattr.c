/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get an attribute of an object.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"
#include <aros/debug.h>

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
	
	storage - Pointer to IPTR the gitten data should be put
		  into.


    RESULT
    	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OOP_SetAttrs().

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    struct pRoot_Get p;
    static OOP_MethodID mid = 0UL;
    
    EnterFunc(bug("OOP_GetAttr())\n"));
    
    if (!mid)
    	mid = OOP_GetMethodID(IID_Root, moRoot_Get);
	
	
    p.mID	= mid;
    p.attrID	= attrID;
    p.storage	= storage;
    
    /* Call the Get() method on the object */
    
    ReturnPtr ("OOP_GetAttr", IPTR, OOP_DoMethod(object, (OOP_Msg)&p));
    
    AROS_LIBFUNC_EXIT
} /* OOP_GetAttr */
