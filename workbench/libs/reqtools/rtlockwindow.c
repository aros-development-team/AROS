/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include "general.h"
#include "rtfuncs.h"

/*****************************************************************************

    NAME */

    AROS_LH1(APTR, rtLockWindow,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 26, ReqTools)

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
	The wait pointer will look exactly like the standard Workbench 2.0
	wait pointer. In combination with PointerX, ClockTick or
	LacePointer the handle will turn.

    EXAMPLE

    BUGS
	none known

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return RTFuncs_rtLockWindow(window);

    AROS_LIBFUNC_EXIT
    
} /* rtLockWindow */
