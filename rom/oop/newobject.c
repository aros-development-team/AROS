/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/

#include <aros/atomic.h>
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
	AROS_LHA(CONST_STRPTR    , classID, A1),
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
    
    struct pRoot_New p;
    OOP_Object *o;

// bug("OOP_NewObject(class=%s, classptr=%p, tags=%p)\n", classID, classPtr, tagList);    
    EnterFunc(bug("OOP_NewObject(classPtr=%p, classID=%s, tagList=%p)\n",
		  classPtr, (classID ? (const char *)classID : "(null)"), tagList));

    if (!classPtr)
    {
	/* If a public ID was given, find pointer to class */
	if (classID)
	    classPtr = OOP_FindClass(classID);
    }

    if (!classPtr)
	ReturnPtr ("OOP_NewObject[No classPtr]", OOP_Object *, NULL);

    /*
     * We don't want the class to be freed while we work on it,
     * so we temporarily increment reference count.
     * Note that real instance counting happens inside rootclass,
     * where it allocates and frees the memory.     
     */
    AROS_ATOMIC_INC(MD(classPtr)->objectcount);

    /* Create a new instance */
    
    D(bug("Creating new instance\n"));

    p.mID = OOP_GetMethodID(IID_Root, moRoot_New);
    p.attrList = tagList;
    
/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/  
    D(bug("mid=%ld\n", p.mID));

    /* Call the New() method of the specified class */
    
    o = (OOP_Object *)OOP_CoerceMethod(classPtr, (OOP_Object *)classPtr, (OOP_Msg)&p);

    /* Release our temporary lock */
    AROS_ATOMIC_DEC(MD(classPtr)->objectcount);

/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/    
    ReturnPtr ("OOP_NewObject", OOP_Object *, o);
    
    
    AROS_LIBFUNC_EXIT
} /* OOP_NewObject */
