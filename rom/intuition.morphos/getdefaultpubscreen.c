/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <string.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(struct Screen *, GetDefaultPubScreen,

         /*  SYNOPSIS */
         AROS_LHA(UBYTE *, nameBuffer, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 97, Intuition)

/*  FUNCTION
    Returns the name of the current default public screen.
    This will be "Workbench" if there is no default public screen.
 
    INPUTS
    nameBuffer - A buffer of length MAXPUBSCREENNAME
 
    RESULT
    None.
 
    NOTES
    Only Public Screen Manager utilities want to use this function
    since it is easy to open a window on the default public screen
    without specifying a name.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    SetDefaultPubScreen(), OpenWindow()
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *defscreen;
    STRPTR  	   name;

    DEBUG_GETDEFAULTPUBSCREEN(dprintf("GetDefaultPubScreen(%s)\n",nameBuffer));

    LockPubScreenList();

    defscreen = GetPrivIBase(IntuitionBase)->DefaultPubScreen;

    if (defscreen)
    {
        name = GetPrivScreen(defscreen)->pubScrNode->psn_Node.ln_Name;
    }
    else
    {
        name = "Workbench";
    }

    if (nameBuffer)
    {
        strcpy(nameBuffer, name);
    }

    UnlockPubScreenList();

    return defscreen;

    AROS_LIBFUNC_EXIT
} /* GetDefaultPubScreen */
