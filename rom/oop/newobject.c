/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"
#include "hash.h"
#include <aros/debug.h>
#define MD(x) ((struct metadata *)x)

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(APTR, OOP_NewObject,

/*  SYNOPSIS */
	AROS_LHA(struct OOP_IClass  *, classPtr, A0),
	AROS_LHA(UBYTE          *, classID, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, OOPBase, 5, OOP)

/*  FUNCTION
	Creates a new object of given class based on the TagItem
	parameters passed.

    INPUTS
    	classPtr - pointer to a class. Use this if the class to
		   create an instance of is private.
	classID  - Public ID of the class to create an instance of.
		   Use this if the class is public.
	tagList  - List of TagItems (creation time attributes),
		   that specifies what initial properties the new
		   object should have.


    RESULT
    	Pointer to the new object, or NULL if object creation failed.

    NOTES
    	You should supply one of classPtr and classID, never
	both. Use NULL for the unspecified one.

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
    
    struct pRoot_New p;
    OOP_Object *o;

// bug("OOP_NewObject(class=%s, classptr=%p, tags=%p)\n", classID, classPtr, tagList);    
    EnterFunc(bug("OOP_NewObject(classPtr=%p, classID=%s, tagList=%p)\n",
    		classPtr, ((classID != NULL) ? classID : (UBYTE *)"(null)"), tagList));
		
    /* Class list is public, so we must avoid race conditions */
    ObtainSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);
    
    if (!classPtr)
    {
	/* If a public ID was given, find pointer to class */
	if (classID) {
	    
	    classPtr = (OOP_Class *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), classID);
	    if (classPtr)
		MD(classPtr)->objectcount ++; /* We don't want the class to be freed while we work on it */
	}
    }
    
    /* Release lock on list */
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);

    if (!classPtr)
	ReturnPtr ("OOP_NewObject[No classPtr]", OOP_Object *, NULL);

    /* Create a new instance */
    
    D(bug("Creating new instance\n"));

    p.mID = OOP_GetMethodID(IID_Root, moRoot_New);
    p.attrList = tagList;
    
/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/  
    D(bug("mid=%ld\n", p.mID));

    /* Call the New() method of the specified class */
    
    D(bug("OOP_Coercemethod: %p\n", classPtr->CoerceMethod));
    o = (OOP_Object *)OOP_CoerceMethod(classPtr, (OOP_Object *)classPtr, (OOP_Msg)&p);
    if (!o)
    {
	MD(classPtr)->objectcount --; /* Object creation failed, release lock */
    }
/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/    
    ReturnPtr ("OOP_NewObject", OOP_Object *, o);
    
    
    AROS_LIBFUNC_EXIT
} /* OOP_NewObject */
