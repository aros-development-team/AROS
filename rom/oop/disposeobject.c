/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"
#define MD(x) ((struct metadata *)x)
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH1(VOID, OOP_DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(OOP_Object  *, obj, A0),

/*  LOCATION */
	struct Library *, OOPBase, 10, OOP)

/*  FUNCTION
	Delete an object that was previously allocated with OOP_NewObject().

    INPUTS
    	obj	- pointer to object to dispose.

    RESULT
    	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    	OOP_NewObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    ULONG mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
    
    EnterFunc(bug("OOP_DisposeObject(classID=%s)\n",
    		OCLASS(obj)->ClassNode.ln_Name));
		
    if (obj == NULL) return;

    OOP_DoMethod(obj, (OOP_Msg)&mid);

        
    ReturnVoid("OOP_DisposeObject");
    
    AROS_LIBFUNC_EXIT
} /* OOP_DisposeObject */
