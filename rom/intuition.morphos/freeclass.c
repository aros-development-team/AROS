/*
	Copyright (C) 1995-2001 AROS - The Amiga Research OS
	$Id$
 
	Desc: Free a BOOPSI Class (obs)
	Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
	NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

#include "maybe_boopsi.h"

AROS_LH1(BOOL, FreeClass,

		 /*  SYNOPSIS */
		 AROS_LHA(struct IClass *, classPtr, A0),

		 /*  LOCATION */
		 struct IntuitionBase *, IntuitionBase, 119, Intuition)

/*  FUNCTION
	Only for class implementatores.
 
	Tries to free a class which has been created with MakeClass() in the
	first place. This will not succeed in all cases: Classes which
	still have living objects or which are still beeing used by subclasses
	can't simply be freed. In this case this call will fail.
 
	Public classes will always be removed with RemoveClass() no matter
	if FreeClass() would succeed or not. This gurantees that after the
	call to FreeClass() no new objects can be created.
 
	If you have a pointer to allocated memory in cl_UserData, you must
	make a copy of that pointer, call FreeClass() and if the call
	succeeded, you may free the memory. If you don't follow these rules,
	you might end up with a class which is partially freed.
 
	INPUTS
	classPtr - The pointer you got from MakeClass().
 
	RESULT
	FALSE if the class couldn't be freed at this time. This can happen
	either if there are still objects from this class or if the class
	is used a SuperClass of at least another class.
 
	TRUE if the class could be freed. You must not use classPtr after
	that.
 
	NOTES
	*Always* calls RemoveClass().
 
	EXAMPLE
	// Free a public class with dynamic memory in cl_UserD
 
	int freeMyClass (Class * cl)
	{
	    struct MyPerClassData * mpcd;
 
	    mpcd = (struct MyPerClassData *)cl->cl_UserData;
 
	    if (FreeClass (cl)
	    {
		FreeMem (mpcd, sizeof (struct MyPerClassData));
		return (TRUE);
	    }
 
	    return (FALSE);
	}
 
	BUGS
 
	SEE ALSO
 
	INTERNALS
	MakeClass(), "Basic Object-Oriented Programming System for Intuition"
	and "boopsi Class Reference" Dokument.
 
	HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if INTERNAL_BOOPSI

	SANITY_CHECKR(classPtr,FALSE)

	/* Make sure no one creates another object from this class. For private
	classes, this call does nothing. */
	RemoveClass (classPtr);

	Forbid();
	if (!classPtr->cl_SubclassCount && !classPtr->cl_ObjectCount)
	{
		classPtr->cl_Super->cl_SubclassCount --;
		Permit();

		FreeMem (classPtr, sizeof (Class));

		return TRUE;
	}
	Permit();

	return FALSE;
#else

/* call boopsi.library function */
	return FreeClass(classPtr);

#endif

	AROS_LIBFUNC_EXIT

} /* FreeClass */
