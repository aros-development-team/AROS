/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$
 
    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

#include "maybe_boopsi.h"

AROS_LH1(void, RemoveClass,

         /*  SYNOPSIS */
         AROS_LHA(struct IClass *, classPtr, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 118, Intuition)

/*  FUNCTION
	Makes a public class inaccessible. This function may be called
	several times on the same class and even if the class never was
	in the public list.
 
    INPUTS
	classPtr - Pointer to the result of MakeClass(). May be NULL.
 
    RESULT
	None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	MakeClass(), FreeClass(), AddClass(), "Basic Object-Oriented
	Programming System for Intuition" and "boopsi Class Reference"
	Dokument.
 
    INTERNALS
 
    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#if INTERNAL_BOOPSI

	ObtainSemaphore( &GetPrivIBase(IntuitionBase)->ClassListLock );

	/* Class there and still/already in the list ? */
	if (classPtr && (classPtr->cl_Flags & CLF_INLIST))
	{
		Remove ((struct Node *)classPtr);
		classPtr->cl_Flags &= ~CLF_INLIST;
	}

	ReleaseSemaphore( &GetPrivIBase(IntuitionBase)->ClassListLock );

#else

	/* Pass to boopsi.library */
	RemoveClass(classPtr);

#endif

	AROS_LIBFUNC_EXIT

} /* RemoveClass */
