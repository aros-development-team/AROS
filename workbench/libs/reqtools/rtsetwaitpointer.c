
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include <proto/intuition.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <intuition/intuition.h>
#include "reqtools_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH1(VOID, rtSetWaitPointer,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */

	struct Library *, RTBase, 20, ReqTools)

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

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct TagItem tags[] = { { WA_BusyPointer, TRUE },
			      { TAG_DONE, NULL } };

    SetWindowPointerA(window, (struct TagItem *)&tags);

    AROS_LIBFUNC_EXIT
} /* rtSetWindowPointer */
