/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/exec.h>
#include <proto/intuition.h>

/*****************************************************************************

    NAME */

      AROS_LH2(VOID, CloseWindowSafely,

/*  SYNOPSIS */ 
      AROS_LHA(struct Window *, window, A0),
      AROS_LHA(LONG, morewindows, A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 50, Arp)

/*  NAME
	  CloseWindowSafely - close a window without GURUing.

    FUNCTION
	  This function	locates	any messages queued to the window
	  message port,	and returns them to the	sender.	 It then
	  closes the window.  This is necessary	for windows which
	  share	a common IDCMP,	otherwise GURU's are likely to occur.
	  If morewindows is NULL, then the msgport is freed also.

    INPUTS
	  window - window to close

	  morewindows -	If NULL, the msgport for this window will be
		  released.

    RESULTS
	  None.
 
    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

    struct MsgPort *port = window->UserPort;
    
    if (port)
    {
        struct IntuiMessage *msg, *nextmsg;
	
        Forbid();
	
	ForeachNodeSafe(&port->mp_MsgList, msg, nextmsg)
	{
	    if (msg->IDCMPWindow == window)
	    {
	        Remove(&msg->ExecMessage.mn_Node);
		ReplyMsg(&msg->ExecMessage);
	    }
	}
	
	if (morewindows)
	{
	    window->UserPort = NULL;
	    ModifyIDCMP(window, 0);
	}
	
	Permit();
	
	CloseWindow(window);
	
    } /* if (port) */

    AROS_LIBFUNC_EXIT
} /* CompareLock */
