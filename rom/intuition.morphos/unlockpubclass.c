/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
 
    Desc: Unlocks the public classes list
    Lang: english
*/
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH0(void, unlockPubClass,

         /*  SYNOPSIS */

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 121, Intuition)

/*  FUNCTION
 
	Unlocks the public classes list.
 
    INPUTS
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
	lockPubClass().
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

	ReleaseSemaphore (&GetPrivIBase(IntuitionBase)->ClassListLock);

	AROS_LIBFUNC_EXIT
} /* unlockPubClass */
