/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <exec/ports.h>
#include <intuition/intuition.h>

        AROS_LH1(struct IntuiMessage *, GT_GetIMsg,

/*  SYNOPSIS */
	AROS_LHA(struct MsgPort *, intuiport, A0),

/*  LOCATION */
	struct Library *, GadToolsBase, 12, GadTools)

/*  FUNCTION
        You must use this function instead of exec.library/GetMsg() to get
        messages from a window, if you are using gadtools gadgets. After you
        are done with reading the message, you have to call GT_ReplyIMsg().

    INPUTS
        intuiport - UserPort of the window

    RESULT
        A pointer to a message or NULL, if there was no message or the messages
        had only a meaning to gadtools.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GT_ReplyIMsg(), exec.library/GetMsg()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntuiMessage *imsg, *gtmsg;

    while ((imsg = (struct IntuiMessage *)GetMsg(intuiport)))
    {
    	if ((gtmsg = GT_FilterIMsg(imsg)))
	{
	    /* msg is for app */
	    imsg = gtmsg;
	    break;
	}
	else
	{
	    /* msg was for gadtools only */
	    ReplyMsg(&imsg->ExecMessage);
	}
    }
    
    return imsg;

    AROS_LIBFUNC_EXIT
    
} /* GT_GetIMsg */
