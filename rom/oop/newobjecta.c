/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a new OOP object
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>
#include <oop/root.h>
// #include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH3(APTR, NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, classPtr, A0),
	AROS_LHA(UBYTE          *, classID, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct Library *, OOPBase, 5, OOP)

/*  FUNCTION
	Creates a new object of given clas based on the TagItem
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
    
    struct P_Root_New p;
    Object *o;
    
    EnterFunc(bug("NewObjectA(classPtr=%p, classID=%s, tagList=%p)\n",
    		classPtr, ((classID != NULL) ? classID : (UBYTE *)"(null)"), tagList));
		
    /* Class list is public, so we must avoid race conditions */
    ObtainSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);
    
    if (!classPtr)
    {
	/* If a public ID was given, find pointer to class */
	classPtr = (Class *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), classID);
	if (classPtr)
	   IntCl(classPtr)->ObjectCount ++; /* We don't want the class to be freed while we work on it */
    }
    
    /* Release lock on list */
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);

    if (!classPtr)
	ReturnPtr ("NewObjectA[No classPtr]", Object *, NULL);

    /* Create a new instance */
    
    D(bug("Creating new instance\n"));

    p.MethodID = (IPTR)M_Root_New;
    p.AttrList = tagList;

    /* Call the New() method of the specified class */
    o = (Object *)CoerceMethodA(classPtr, (Object *)classPtr, (Msg)&p);
    if (!o)
    {
	IntCl(classPtr)->ObjectCount --; /* Object creation failed, release lock */
    }
    ReturnPtr ("NewObjectA", Object *, o);
    
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
