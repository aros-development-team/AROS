
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

	struct Library *, RTBase, 28, ReqTools)

/*  FUNCTION

    Unlock a window previously locked with rtLockWindow(). The window will
    once again accept user input.

    INPUTS

    window  --  pointer to the window to be unlocked


    RESULT


    NOTES

    The mouse pointer has to be set back manually for now.

    EXAMPLE

    BUGS

    SEE ALSO

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
