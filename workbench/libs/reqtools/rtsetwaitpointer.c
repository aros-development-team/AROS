/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/intuition.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <intuition/intuition.h>
#include "general.h"
#include "reqtools_intern.h"
#include "rtfuncs.h"

/*****************************************************************************

    NAME */

    AROS_LH1(VOID, rtSetWaitPointer,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */

	struct ReqToolsBase *, ReqToolsBase, 19, ReqTools)

/*  FUNCTION
	Change the pointer image to that of a wait pointer. This function should
	be called when your program is busy for a longer period of time.

	It is recommended you call this function before calling any of the
	requester functions.  This way if the user clicks in your window he will
	know he must respond to the requester before doing anything else.  Also
	see the RT_WaitPointer tag for an automatic way of setting the wait
	pointer.  If you are using ReqTools V38+ check out the RT_LockWindow tag!

    INPUTS
	window  --  pointer to the window to receive the wait pointer

    RESULT
	none

    NOTES
	The wait pointer will look exactly like the standard Workbench 2.0
	wait pointer. In combination with PointerX, ClockTick or
	LacePointer the handle will turn.

    EXAMPLE

    BUGS
	none known

    SEE ALSO
	rtEZRequestA(), (RT_WaitPointer and RT_LockWindow tags), rtLockWindow()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    RTFuncs_rtSetWaitPointer(window);
    
    AROS_LIBFUNC_EXIT
    
} /* rtSetWaitPointer */
