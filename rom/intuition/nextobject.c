/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Iterate through a list of objects
    Lang: english
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>
#include <proto/boopsi.h>

	AROS_LH1(APTR, NextObject,

/*  SYNOPSIS */
	AROS_LHA(APTR, objectPtrPtr, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 111, Intuition)

/*  FUNCTION
	Use this function to iterate through a list of BOOPSI objects.
	You may do whatever you want with the object returned, even
	remove it from the list or disposing it and then continue to
	iterate thought the list.

    INPUTS
	objectPtrPtr - the pointer to a variable. This must be the same
	    variable, as long as you iterate though the same list. This
	    variable must initially be filled with the lh_Head of a list.

    RESULT
	A BOOPSI object, which can be manipulated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Pass call to the boopsi.library */
    return NextObject(objectPtrPtr);

#if 0
    APTR oldobject;
    struct _Object *nextobject;
    
    oldobject = (APTR)(*((Object **)objectPtrPtr));
    
    if (oldobject)
    {
    	nextobject = (struct _Object *)_OBJECT(oldobject)->o_Node.mln_Succ;
    
    	if (nextobject)
    	{
	    *((Object **)objectPtrPtr) = BASEOBJECT(nextobject);
    	} 
    	else
    	{
            *((Object **)objectPtrPtr) = NULL;
    	}
    }

    return oldobject;
#endif

    AROS_LIBFUNC_EXIT
} /* NextObject */
