
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <proto/intuition.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH1(APTR, rtLockWindow,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */

	struct Library *, RTBase, 27, ReqTools)

/*  FUNCTION

    Lock a window so it will no longer accept any user input.  The only
    functions left to the user are depth arrangement and window dragging.
    All gadgets will be un-selectable and the window can not be resized.
    It will also get the standard wait pointer set.  The pointer at the
    time of locking will be restored when the window is unlocked.
    
    You may nest calls to rtLockWindow() and rtUnlockWindow().  Just make
    sure you unlock the window in the correct (opposite) order.
    
    See the RT_LockWindow tag for an automatic way of locking your window.
    
    Use this function (and rtUnlockWindow()) instead of rtSetWaitPointer().
    
    INPUTS

    window  --  pointer to the window to be locked

    RESULT

    a pointer to a (private) window lock.  You must pass this to
    rtUnlockWindow() to unlock the window again. Never mind if this is NULL.
    This means there was not enough memory and the window will not be locked.
    There is no sense in reporting this, just carry on and pass the NULL
    window lock to rtUnlockWindow().

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct rtWindowLock *winLock;

    /* Is this window already locked? */
    if(window->FirstRequest != NULL)
    {
	struct rtWindowLock *wLock = (struct rtWindowLock *)window->FirstRequest;

	while(wLock != NULL)
	{
	    if(wLock->rtwl_Magic ==  ('r' << 24 | 't' << 16 | 'L' << 8 | 'W'))
	    {
		if(wLock->rtwl_RequesterPtr == wLock)
		{
		    /* Window was already locked */
		    wLock->rtwl_LockCount++;

		    return wLock;
		}
	    }

	    wLock = (struct rtWindowLock *)wLock->rtwl_Requester.OlderRequest;
	}
    }

    winLock = (struct rtWindowLock *)AllocVec(sizeof(struct rtWindowLock),
					      MEMF_CLEAR);
    
    /* No memory? */
    if(winLock == NULL)
	return NULL;
    
    winLock->rtwl_Magic = 'r' << 24 | 't' << 16 | 'W' << 8 | 'L';
    winLock->rtwl_RequesterPtr = winLock;
    
    winLock->rtwl_MinHeight = window->MinHeight;
    winLock->rtwl_MaxHeight = window->MaxHeight;
    winLock->rtwl_MinWidth  = window->MinWidth;
    winLock->rtwl_MaxWidth  = window->MaxWidth;

    WindowLimits(window, window->Width, window->Height,
		 window->Width, window->Height);
    
    winLock->rtwl_ReqInstalled = Request((struct Requester *)winLock, window);
    
    winLock->rtwl_Pointer = window->Pointer;
    winLock->rtwl_PtrHeight = window->PtrHeight;
    winLock->rtwl_PtrWidth = window->PtrWidth;
    
    rtSetWaitPointer(window);
    
    return (APTR)winLock;

    AROS_LIBFUNC_EXIT
} /* rtLockWindow */
