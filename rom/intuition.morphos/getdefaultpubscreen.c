/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
 
    Desc: Intuition function GetDefaultPubScreen()
    Lang: english
*/
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
 
    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    STRPTR name;
    struct Screen *defscreen;

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
