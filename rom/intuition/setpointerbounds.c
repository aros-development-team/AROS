/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.

*/

#include "intuition_intern.h"
#include "inputhandler.h"

AROS_LH4(ULONG, SetPointerBounds,
         /*  SYNOPSIS */
         AROS_LHA(struct Screen    *, screen  , A0),
	 AROS_LHA(struct Rectangle *, rect    , A1),
         AROS_LHA(ULONG             , reserved, D0),
         AROS_LHA(struct TagItem   *, tags    , A2),
         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 160, Intuition)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
   
    (void)screen;
    (void)reserved;
    
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    if (!iihd->MouseBoundsActiveFlag)
    {
    	if (rect)
	{
    	    iihd->MouseBoundsActiveFlag = TRUE;
	    iihd->MouseBoundsKillTimer  = 5; /* 1 sec */
	    iihd->MouseBoundsLeft 	= rect->MinX;
	    iihd->MouseBoundsTop  	= rect->MinY;
	    iihd->MouseBoundsRight      = rect->MaxX;
	    iihd->MouseBoundsBottom     = rect->MaxY;
	}		
    }
    else if (iihd->MouseBoundsKillTimer)
    {
	if (rect)
	{
    	    /* Reset timer */
    	    iihd->MouseBoundsKillTimer  = 5; /* 1 sec */
	    iihd->MouseBoundsLeft 	= rect->MinX;
	    iihd->MouseBoundsTop  	= rect->MinY;
	    iihd->MouseBoundsRight      = rect->MaxX;
	    iihd->MouseBoundsBottom     = rect->MaxY;
	}
	else
	{
	    iihd->MouseBoundsKillTimer = 0;
	    iihd->MouseBoundsActiveFlag = FALSE;
	}	
    }
    
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}
