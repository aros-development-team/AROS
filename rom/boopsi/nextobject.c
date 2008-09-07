/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Iterate through a list of objects
    Lang: english
*/
#include <exec/types.h>
#include <exec/nodes.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/boopsi.h>

	AROS_LH1(APTR, NextObject,

/*  SYNOPSIS */
	AROS_LHA(APTR, objectPtrPtr, A0),

/*  LOCATION */
	struct Library *, BOOPSIBase, 12, BOOPSI)

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
	NewObjectA(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    APTR oldobject;
    struct _Object *nextobject;
    
    nextobject = (struct _Object *)(*((struct _Object **)objectPtrPtr))->o_Node.mln_Succ;
    if (nextobject)
    {
    	oldobject = (Object *)BASEOBJECT(*((struct _Object **)objectPtrPtr));
    	*((struct _Object **)objectPtrPtr) = nextobject;
    }
    else
    {
    	oldobject = NULL;
    }

    return oldobject;
    AROS_LIBFUNC_EXIT
} /* NextObject */
