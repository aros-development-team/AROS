/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(void, SetDefaultPubScreen,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 90, Intuition)

/*  FUNCTION
        Specifies the default public screen for visitor windows to open up on.
        The screen is used when a requested public screen is not available
        and the FALLBACK option is enabled or when the visitor window asks for
        the default public screen.

    INPUTS
        name - The name of the public screen that should be used as default,
               or NULL to specify the Workbench screen.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        OpenWindow(), OpenScreen()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    //ignored for defpubscreen patch
    if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN) return;

    if (name)
    {
        struct PubScreenNode * psn;

        if ((psn = (struct PubScreenNode *)FindName (LockPubScreenList(), name)))
            GetPrivIBase(IntuitionBase)->DefaultPubScreen = psn->psn_Screen;

        UnlockPubScreenList();
    }
    else
        GetPrivIBase(IntuitionBase)->DefaultPubScreen = NULL;

    AROS_LIBFUNC_EXIT
} /* SetDefaultPubScreen */
