/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function CloseWorkBench()
    Lang: english
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/*****************************************************************************

    NAME */

    AROS_LH0(LONG, CloseWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
    struct IntuitionBase *, IntuitionBase, 13, Intuition)

/*  FUNCTION
	Attempt to close the Workbench screen:
	- Check for open application windows. return FALSE if there are any
	- Clean up all special buffers
	- Close the Workbench screen
	- Make the Workbench program mostly inactive
	  (disk activity will still be monitored)
	- Return TRUE

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen *wbscreen;
    BOOL    	   retval = TRUE;

    LockPubScreenList();
 
    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;
 
    if (!wbscreen)
    {
    	retval = FALSE;
    }
    else if (GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount != 0)
    {
    	retval = FALSE;
    }
    
    UnlockPubScreenList();
    
    if (!retval) return FALSE;
    
    /* If there is a Workbench process running, tell it to close it's windows. */

    /* Don't call this function while pub screen list is locked! */
    TellWBTaskToCloseWindows(IntuitionBase);
    
    LockPubScreenList();
    
    wbscreen = GetPrivIBase(IntuitionBase)->WorkBench;
    
    /* Try to close the Workbench screen, if there is any. */
    if( wbscreen != NULL )
    {
        if( CloseScreen( wbscreen ) == TRUE )
	{
            GetPrivIBase(IntuitionBase)->WorkBench = NULL;
        }
	else
	{
	    /* Grrr ... closing screen failed. I inc psn_VisitorCount by hand here,
	       to avoid that someone else can kill it, because I must tell Workbench
	       task that it shall open its windows again */
	       
            GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount++;
	    retval = FALSE;	    
	}
    }
    else
    {
    	retval = FALSE;
    }
    UnlockPubScreenList();

    if (wbscreen && (retval == FALSE))
    {
    	/* Don't call this function while pub screen list is locked! */
    	TellWBTaskToOpenWindows(IntuitionBase);
	
	/* Fix psn_VisitorCount which was increased above */
	LockPubScreenList();
        GetPrivScreen(wbscreen)->pubScrNode->psn_VisitorCount--;
	UnlockPubScreenList();
    }
    
    return retval;

    AROS_LIBFUNC_EXIT
    
} /* CloseWorkBench */
