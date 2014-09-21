/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set attributes of an object.
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include "intern.h"
#include <oop/static_mid.h>

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH2(IPTR, OOP_SetAttrs,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object	*, object, A0),
	AROS_LHA(struct TagItem	*, attrList, A1),

/*  LOCATION */
	struct Library *, OOPBase, 17, OOP)

/*  FUNCTION
	Sets the object's attributes as specified in the
	supplied taglist.

    INPUTS
    	object	- pointer to a object in whih we
		  want to set attributes.
		 
	tagList -  List of attributes and their new values.

    RESULT
    	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OOP_DisposeObject()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct pRoot_Set p;
    struct IntOOPBase *iOOPBase = (struct IntOOPBase *)OOPBase;
    
    EnterFunc(bug("OOP_SetAttrs())\n"));
    ASSERT_VALID_PTR(object);
    ASSERT_VALID_PTR_OR_NULL(attrList);
    
    if (!iOOPBase->ob_mRoot_Set)
        iOOPBase->ob_mRoot_Set = OOP_GetMethodID(IID_Root, moRoot_Set);

    p.mID	= iOOPBase->ob_mRoot_Set;
    p.attrList	= attrList;

    /* Call the Set() method on the object */ 
    ReturnPtr ("OOP_SetAttrs", IPTR, OOP_DoMethod(object, (OOP_Msg)&p));
    
    AROS_LIBFUNC_EXIT
} /* OOP_SetAttrs */
