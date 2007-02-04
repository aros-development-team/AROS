/*
    Copyright  1995-2003, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(UWORD, PubScreenStatus,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, Scr        , A0),
         AROS_LHA(UWORD          , StatusFlags, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 92, Intuition)

/*  FUNCTION
 
    Change the status flags for a given public screen. 
 
    INPUTS
 
    Scr          --  The screen the flags of which to change.
    StatusFlags  --  The new values for the flags, see <intuition/screens.h>
                     for further information on the flag bits.
 
    RESULT
 
    Clears bit 0 if the screen wasn't public or if it was impossible
    to make private (PSNF_PRIVATE) as visitor windows are open on it.
    The other bits in the return value are reserved for future use.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    OpenScreen()
 
    INTERNALS
 
    HISTORY
        21-06-98    SDuvan  Implemented
 
*****************************************************************************/
#define GPB(x) GetPrivIBase(x)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ULONG  retval;

    SANITY_CHECKR(Scr,FALSE)

    LockPubScreenList();

    if (GetPrivScreen(Scr)->pubScrNode == NULL ||
        GetPrivScreen(Scr)->pubScrNode->psn_VisitorCount != 0)
    {
        retval = 0x0;
    }
    else
    {
        GetPrivScreen(Scr)->pubScrNode->psn_Flags = StatusFlags;
        retval = 0x1;
    }

    UnlockPubScreenList();

    FireScreenNotifyMessage((IPTR) Scr, SNOTIFY_PUBSCREENSTATE, IntuitionBase);

    return retval;

    AROS_LIBFUNC_EXIT
} /* PubScreenStatus */
