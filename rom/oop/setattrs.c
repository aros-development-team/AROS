/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set attributes of an object.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include "intern.h"

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
		 
	tagList -  List of attributes an their new values.


    RESULT
    	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OOP_DisposeObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    struct pRoot_Set p;
    static OOP_MethodID mid = 0UL;
    
    EnterFunc(bug("OOP_SetAttrs())\n"));
    ASSERT_VALID_PTR(object);
    ASSERT_VALID_PTR(attrList);
    
    if (!mid)
    	mid = OOP_GetMethodID(IID_Root, moRoot_Set);

    p.mID	= mid;
    p.attrList	= attrList;

    /* Call the Get() method on the object */ 
    ReturnPtr ("OOP_SetAttrs", IPTR, OOP_DoMethod(object, (OOP_Msg)&p));
    
    AROS_LIBFUNC_EXIT
} /* OOP_SetAttrs */
