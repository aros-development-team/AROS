/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Find a BOOPSI Class in the class list.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>

#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/boopsi.h>    
	
	AROS_LH1(struct IClass *, FindClass,

/*  SYNOPSIS */
	AROS_LHA(ClassID,	classID, A0),

/*  LOCATION */
	struct Library *, BOOPSIBase, 7, BOOPSI)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, BOOPSIBase)

    Class * classPtr = NULL;

    if (!classID)
	return NULL;

    /* Lock the list */
    ObtainSemaphoreShared (&GetBBase(BOOPSIBase)->bb_ClassListLock);

    /* Search for the class */
    ForeachNode (&GetBBase(BOOPSIBase)->bb_ClassList, classPtr)
    {
	if (!strcmp (classPtr->cl_ID, classID))
	    goto found;
    }

    classPtr = NULL; /* Nothing found */

found:
    /* Unlock list */
    ReleaseSemaphore (&GetBBase(BOOPSIBase)->bb_ClassListLock);

    return classPtr;

    AROS_LIBFUNC_EXIT
}

