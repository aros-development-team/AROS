/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Find a BOOPSI Class in the class list.
*/

#include <string.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include "intuition_intern.h"

#undef SDEBUG
#define SDEBUG 0
#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************
 
    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

AROS_LH1(struct IClass *, FindClass,

         /*  SYNOPSIS */
         AROS_LHA(ClassID,  classID, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 112, Intuition)

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

    DEBUG_FINDCLASS(dprintf("FindClass: ClassID <%s>\n",
                            classID ? classID : (UBYTE*)"NULL"));

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

    DEBUG_FINDCLASS(dprintf("FindClass: return 0x%lx\n", classPtr));

    ReturnPtr("intuition_boopsi::FindClass()", struct IClass *, classPtr);

    AROS_LIBFUNC_EXIT
}

