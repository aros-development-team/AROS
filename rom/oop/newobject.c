/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"
#define MD(x) ((struct metadata *)x)

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(APTR, NewObject,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, classPtr, A0),
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
	DisposeObject()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library*,OOPBase)
    
    struct pRoot_New p;
    Object *o;

// kprintf("NewObject(class=%s, classptr=%p, tags=%p)\n", classID, classPtr, tagList);    
    EnterFunc(bug("NewObject(classPtr=%p, classID=%s, tagList=%p)\n",
    		classPtr, ((classID != NULL) ? classID : (UBYTE *)"(null)"), tagList));
		
    /* Class list is public, so we must avoid race conditions */
    ObtainSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);
    
    if (!classPtr)
    {
	/* If a public ID was given, find pointer to class */
	if (classID) {
	    
	    classPtr = (Class *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), classID);
	    if (classPtr)
		MD(classPtr)->objectcount ++; /* We don't want the class to be freed while we work on it */
	}
    }
    
    /* Release lock on list */
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);

    if (!classPtr)
	ReturnPtr ("NewObject[No classPtr]", Object *, NULL);

    /* Create a new instance */
    
    D(bug("Creating new instance\n"));

    p.mID = GetMethodID(IID_Root, moRoot_New);
    p.attrList = tagList;
    
/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/    
    D(bug("mid=%ld\n", p.mID));

    /* Call the New() method of the specified class */
    
    D(bug("Coercemethod: %p\n", classPtr->CoerceMethod));
    o = (Object *)CoerceMethod(classPtr, (Object *)classPtr, (Msg)&p);
    if (!o)
    {
	MD(classPtr)->objectcount --; /* Object creation failed, release lock */
    }
/*    print_table(GetOBase(OOPBase)->ob_IIDTable, GetOBase(OOPBase));
*/    
    ReturnPtr ("NewObject", Object *, o);
    
    
    AROS_LIBFUNC_EXIT
} /* NewObject */
