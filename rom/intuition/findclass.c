/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Find a BOOPSI Class in the class list.
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <exec/lists.h>
#include <proto/exec.h>

#include "intuition_intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>    
	
	AROS_LH1(struct IClass *, FindClass,

/*  SYNOPSIS */
	AROS_LHA(ClassID,	classID, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 147, Intuition)

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
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *, IntuitionBase)

    Class * classPtr = NULL;

    EnterFunc(bug("intuition_boopsi::FindClass()\n"));

    if (!classID)
	return NULL;

    D(bug("class to find: \"%s\"\n", classID));

    /* Lock the list */
    ObtainSemaphoreShared (&GetPrivIBase(IntuitionBase)->ClassListLock);

    /* Search for the class */
    ForeachNode (&GetPrivIBase(IntuitionBase)->ClassList, classPtr)
    {
        D(bug("+\"%s\"\n", classPtr->cl_ID));
	if (!strcmp (classPtr->cl_ID, classID))
	    goto found;
    }

    classPtr = NULL; /* Nothing found */
    D(bug("class not found!\n"));

found:
    /* Unlock list */
    ReleaseSemaphore (&GetPrivIBase(IntuitionBase)->ClassListLock);

    ReturnPtr("intuition_boopsi::FindClass()", struct IClass *, classPtr);
    
    AROS_LIBFUNC_EXIT
}

