/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
 
    Desc: Locks the public classes list
    Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH0(void, lockPubClass,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 120, Intuition)

/*  FUNCTION
 
	Locks the public classes list.
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
	unlockPubClass().
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->ClassListLock);

	AROS_LIBFUNC_EXIT
} /* unlockPubClass */
