
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/reqtools.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH2(VOID, rtUnlockWindow,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window     , A0),
	AROS_LHA(APTR           , windowlock , A1),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 27, ReqTools)

/*  FUNCTION
	Unlock a window previously locked with rtLockWindow(). The window
	will once again accept user input and will get its original mouse
	pointer back (default or custom).

	Under Kickstart V39 or higher the original window pointer will not
	be restored if it was set using SetWindowPointer(). You will have to
	restore the pointer yourself in this case.

    INPUTS
	window - pointer to the window to be unlocked.
	windowlock - the windowlock pointer returned by rtLockWindow(), may
	    be NULL.

    RESULT
	none

    NOTES
	The mouse pointer has to be set back manually for now.

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtLockWindow()

    INTERNALS
	Needs semaphores to avoid race condition problems for multithreaded
	applications.

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct rtWindowLock *wLock = (struct rtWindowLock *)windowlock;

    if(wLock == NULL)
	return;

    if(wLock->rtwl_LockCount != 0)
    {
	wLock->rtwl_LockCount--;
    }
    else
    {
	struct TagItem tags[] = { { WA_Pointer, (IPTR)wLock->rtwl_Pointer },
				  { TAG_DONE  , NULL } };

	SetWindowPointerA(window, (struct TagItem *)&tags);

	WindowLimits(window, wLock->rtwl_MinWidth, wLock->rtwl_MaxWidth,
		     wLock->rtwl_MinHeight, wLock->rtwl_MaxHeight);

	FreeVec(wLock);
    }
    
    AROS_LIBFUNC_EXIT
} /* rtUnlockWindow */
