/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(BOOL, ResetMenuStrip,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Menu *, menu, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 117, Intuition)

/*  FUNCTION
    Works like a "fast" SetMenuStrip() as it doesn't check Menu or
    calculate internal values before attaching the Menu to the Window.
    Use this function only if the Menu has been added before by
    SetMenuStrip() and you changed nothing in the struct except
    CHECKED and ITEMENABLED flags.
 
    INPUTS
    window - The window to add the MenuStrip to
    menu   - The menu to be added to the window above.
 
    RESULT
    Always TRUE.
 
    NOTES
        Yes, I do repeat it again:
    Use this function only if the Menu has been added before by
    SetMenuStrip() and you changed nothing in the struct except
    CHECKED and ITEMENABLED flags.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    SetMenuStrip(), ClearMenuStrip()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    SANITY_CHECKR(window,TRUE)

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
    window->MenuStrip = menu;
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ResetMenuStrip */
