/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new BOOPSI object
    Lang: english
*/
#include <exec/lists.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

#include "maybe_boopsi.h"

	AROS_LH3(APTR, NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(struct IClass  *, classPtr, A0),
	AROS_LHA(UBYTE          *, classID, A1),
	AROS_LHA(struct TagItem *, tagList, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 106, Intuition)

/*  FUNCTION
	Use this function to create BOOPSI objects (BOOPSI stands for
	"Basic Object Oriented Programming System for Intuition).

	You may specify a class either by it's name (if it's a public class)
	or by a pointer to its definition (if it's a private class). If
	classPtr is NULL, classID is used.

    INPUTS
	classPtr - Pointer to a private class (or a public class if you
		happen to have a pointer to it)
	classID - Name of a public class
	tagList - Initial attributes. Read the documentation of the class
		carefully to find out which attributes must be specified
		here and which can.

    RESULT
	A BOOPSI object which can be manipulated with general functions and
	which must be disposed with DisposeObject() later.

    NOTES
	This functions send OM_NEW to the dispatcher of the class.

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
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if INTERNAL_BOOPSI
 
    Object * object;

    EnterFunc(bug("intuition::NewObjectA()\n"));
 
    /* Lock the list */
    ObtainSemaphoreShared (&GetPrivIBase(IntuitionBase)->ClassListLock);

    /* No classPtr ? */
    if (!classPtr)
	classPtr = FindClass (classID);

    if (classPtr)
    {
    	/* To prevent class from being flushed we increase cl_ObjectCount [1] */
	
    	#warning "Use atomic macros for this once we have some"
    	Forbid();
	classPtr->cl_ObjectCount ++;
	Permit();
    }
    
    ReleaseSemaphore (&GetPrivIBase(IntuitionBase)->ClassListLock);

    if (!classPtr)
	return (NULL); /* Nothing found */

    D(bug("classPtr: %p\n", classPtr));

    /* Try to create a new object. This will also increase cl_ObjectCount in rootclass */
    object = (Object *) CoerceMethod (classPtr, (Object *)classPtr, OM_NEW, (IPTR)tagList, TAG_DONE);
    
    /* Now we can undo what we did here [1] */
    
    #warning "Use atomic macros for this once we have some"
    Forbid();
    classPtr->cl_ObjectCount --;
    Permit();
    
    ReturnPtr("intuition::NewObjectA()", Object *, object);

#else

    /* Pass call to the BOOPSI.library */
    return NewObjectA(classPtr, classID, tagList);

#endif

    AROS_LIBFUNC_EXIT
    
} /* NewObjectA */
