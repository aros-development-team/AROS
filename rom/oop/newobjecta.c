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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	DisposeObject(), SetAttrs(), GetAttr(), MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

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
    
    EnterFunc(bug("NewObjectA(classPtr=%p, classID=%p, tagList=%p)\n",
    		classPtr, classID, tagList));

    ObtainSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);
    
    if (!classPtr)
    {
	
	classPtr = (Class *)FindName((struct List *)&(GetOBase(OOPBase)->ob_ClassList), classID);
	if (classPtr)
	   IntCl(classPtr)->ObjectCount ++; /* We don't want the class to be freed while we work on it */
    }
    
    ReleaseSemaphore(&GetOBase(OOPBase)->ob_ClassListLock);

    if (!classPtr)
	return (NULL);

    /* Create a new instance */
    
    D(bug("Creating new instance\n"));

    p.MethodID = (IPTR)M_Root_New;
    p.AttrList = tagList;

    o = (Object *)CoerceMethodA(classPtr, (Object *)classPtr, (Msg)&p);
    if (!o)
    {
	IntCl(classPtr)->ObjectCount --; /* Object creation failed, release lock */
    }
    ReturnPtr ("NewObjectA", Object *, o);
    
    
    AROS_LIBFUNC_EXIT
} /* NewObjectA */
