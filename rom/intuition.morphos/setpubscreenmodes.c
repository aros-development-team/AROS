/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(UWORD, SetPubScreenModes,

         /*  SYNOPSIS */
         AROS_LHA(UWORD, modes, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 91, Intuition)

/*  FUNCTION
 
    Specify global intuition public screen handling.
 
    INPUTS
 
    modes  --  The new set of flags to consider. Currently defined flags are:
               SHANGHAI       Workbench windows are opened on the default
                  public screen.
           POPPUBSCREEN   When a visitor window opens on a public screen,
                  the screen is brought to front.
 
    RESULT
 
    The flags set before the change was made.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    OpenScreen()
 
    INTERNALS
 
    HISTORY
 
    24-06-98    SDuvan  Implemented
 
*****************************************************************************/
#define GPB(x) GetPrivIBase(x)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    UWORD retval;

    Forbid();
    retval = GPB(IntuitionBase)->pubScrGlobalMode;
    GPB(IntuitionBase)->pubScrGlobalMode = modes;
    Permit();

    return retval;

    AROS_LIBFUNC_EXIT
} /* SetPubScreenModes */
