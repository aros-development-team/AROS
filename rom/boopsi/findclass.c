/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Find a BOOPSI Class in the class list.
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>

#include "intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

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

    Class * classPtr = NULL;

    EnterFunc(bug("boopsi::FindClass()\n"));

    if (!classID)
	return NULL;

    D(bug("class to find: \"%s\"\n", classID));

    /* Lock the list */
    ObtainSemaphoreShared (&GetBBase(BOOPSIBase)->bb_ClassListLock);

    /* Search for the class */
    ForeachNode (&GetBBase(BOOPSIBase)->bb_ClassList, classPtr)
    {
        D(bug("+\"%s\"\n", classPtr->cl_ID));
	if (!strcmp (classPtr->cl_ID, classID))
	    goto found;
    }

    classPtr = NULL; /* Nothing found */
    D(bug("class not found!\n"));

found:
    /* Unlock list */
    ReleaseSemaphore (&GetBBase(BOOPSIBase)->bb_ClassListLock);

    ReturnPtr("boopsi::FindClass()", struct IClass *, classPtr);
    AROS_LIBFUNC_EXIT
}

