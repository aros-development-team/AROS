
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

    AROS_LH1(VOID, rtCloseWindowSafely,

/*  SYNOPSIS */

	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */

	struct Library *, RTBase, 26, ReqTools)

/*  FUNCTION

    Closes a window which shares its IDCMP port with another window.  All the
    pending messages (concerning this window) on the port will be removed and
    the window will be closed.
    
    Do not use this function to close windows which have an IDCMP port set up
    by Intuition.  If you do the port will be left in memory!
    
    If you intend to open a lot of windows all sharing the same IDCMP port it
    is easiest if you create a port yourself and open all windows with
    newwin.IDCMPFlags set to 0 (this tells Intuition to NOT set up an IDCMP
    port).  After opening the window set the win->UserPort to your message
    port and call ModifyIDCMP to set your IDCMP flags.
    
    When you then receive messages from intuition check the imsg->IDCMPWindow
    field to find out what window they came from and act upon them.
    
    When closing your windows call rtCloseWindowSafely() for all of them and
    delete your message port.

    INPUTS

    window  --  pointer to the window to be closed

    RESULT

    NOTES

    This function is for the advanced ReqTools user.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntuiMessage *msg;

    Forbid();

    if(window->UserPort != NULL)
    {
	while((msg = (struct IntuiMessage *)GetMsg(window->UserPort)) != NULL)
	{
	    if(msg->IDCMPWindow == window)
	    {
		Remove((struct Node *)msg);
		ReplyMsg((struct Message *)msg);
	    }
	}
    }

    window->UserPort = NULL;

    ModifyIDCMP(window, 0);

    Permit();

    CloseWindow(window);

    AROS_LIBFUNC_EXIT
} /* rtCloseWindowSafely*/
