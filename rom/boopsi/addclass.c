/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Makes a class publically available.
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/boopsi.h>

	AROS_LH1(void, AddClass,

/*  SYNOPSIS */
	AROS_LHA(struct IClass *, classPtr, A0),

/*  LOCATION */
	struct Library *, BOOPSIBase, 5, BOOPSI)

/*  FUNCTION
	Makes a class publically usable. This function must not be called
	before MakeClass().

    INPUTS
	class - The result of MakeClass()

    RESULT
	None.

    NOTES
	Do not use this function for private classes.
    
    EXAMPLE

    BUGS
	There is no protection against creating multiple classes with
	the same name yet. The operation of the system is undefined
	in this case.

    SEE ALSO
	MakeClass(), FreeClass(), RemoveClass(), "Basic Object-Oriented
	Programming System for Intuition" and "boopsi Class Reference"

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore (&GetBBase(BOOPSIBase)->bb_ClassListLock);
    AddTail (	(struct List *)&GetBBase(BOOPSIBase)->bb_ClassList,
		(struct Node *)classPtr );
    classPtr->cl_Flags |= CLF_INLIST;
    ReleaseSemaphore (&GetBBase(BOOPSIBase)->bb_ClassListLock);

    AROS_LIBFUNC_EXIT
} /* AddClass */
