/*
	(C) 1997-2001 AROS - The Amiga Research OS
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

#include "maybe_boopsi.h"

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

#if INTERNAL_BOOPSI

	struct _Object *nextobject;
	APTR    	    oldobject;

	DEBUG_NEXTOBJECT(dprintf("NextObject: optr 0x%lx\n",
	                         objectPtrPtr));

	IntuitionBase = IntuitionBase;  /* shut up the compiler */

	SANITY_CHECKR(objectPtrPtr,FALSE)

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

#else
	/* Pass call to the boopsi.library */
	return NextObject(objectPtrPtr);

#endif

	AROS_LIBFUNC_EXIT

} /* NextObject */
