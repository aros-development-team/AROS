/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/28 17:55:36  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <intuition/classes.h>
	#include <clib/intuition_protos.h>

	__AROS_LH1(void, RemoveClass,

/*  SYNOPSIS */
	__AROS_LHA(struct IClass *, classPtr, A0),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Klasse da und noch/schon in der Liste ? */
    if (classPtr && (classPtr->cl_Flags & CLF_INLIST))
    {
	Forbid ();

	Remove ((struct Node *)classPtr);

	Permit ();

	classPtr->cl_Flags &= ~CLF_INLIST;
    }

    __AROS_FUNC_EXIT
} /* RemoveClass */
