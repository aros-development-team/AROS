/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/boopsi.h>

	AROS_LH1(void, RemoveClass,

/*  SYNOPSIS */
	AROS_LHA(struct IClass *, classPtr, A0),

/*  LOCATION */
	struct Library *, BOOPSIBase, 13, BOOPSI)

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

    /* Klasse da und noch/schon in der Liste ? */
    if (classPtr && (classPtr->cl_Flags & CLF_INLIST))
    {
	/* Changed to Semaphores during boopsi.library creation */
	ObtainSemaphore( &GetBBase(BOOPSIBase)->bb_ClassListLock );
	Remove ((struct Node *)classPtr);
	ReleaseSemaphore( &GetBBase(BOOPSIBase)->bb_ClassListLock );

	classPtr->cl_Flags &= ~CLF_INLIST;
    }

    AROS_LIBFUNC_EXIT
} /* RemoveClass */
