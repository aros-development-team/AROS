/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Makes a class publically available.
*/

#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

AROS_LH1(void, AddClass,

         /*  SYNOPSIS */
         AROS_LHA(struct IClass *, classPtr, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 114, Intuition)

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_ADDCLASS(dprintf("AddClass: class 0x%lx super 0x%lx dispatcher 0x%lx ID <%s>\n",
                           classPtr, classPtr->cl_Super, classPtr->cl_Dispatcher, classPtr->cl_ID));

    SANITY_CHECK(classPtr)

    ObtainSemaphore (&GetPrivIBase(IntuitionBase)->ClassListLock);
    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->ClassList, (struct Node *)classPtr);
    classPtr->cl_Flags |= CLF_INLIST;
    ReleaseSemaphore (&GetPrivIBase(IntuitionBase)->ClassListLock);

    AROS_LIBFUNC_EXIT
} /* AddClass() */
