/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include <exec/exec.h>

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(struct IntuiMessage *, AllocIntuiMessage,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 148, Intuition)

/*  FUNCTION
    Private to AROS: allocate an IntuiMessage. IntuiMessage->Window
    will be set to window by this function.
 
    INPUTS
    window - The window to which the IntuiMessage will be sent
 
    RESULT
    an allocated IntuiMessage structure. Must be freed with FreeIntuiMessage
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct IntuiMessage *msg;

#ifdef __MORPHOS__
    ASSERT_VALID_PTR(window);
    SANITY_CHECKR(window,0)
#else
    ASSERT_VALID_PTR_OR_NULL(window);
#endif

    if ((msg = AllocPooled(GetPrivIBase(IntuitionBase)->IDCMPPool,sizeof(struct IntIntuiMessage))))
    {
        memclr(msg,sizeof(struct IntIntuiMessage));

        msg->ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
        msg->ExecMessage.mn_Length       = sizeof(struct ExtIntuiMessage);
        msg->ExecMessage.mn_ReplyPort    = window ? window->WindowPort : GetPrivIBase(IntuitionBase)->IntuiReplyPort;

        msg->IDCMPWindow            	 = window;
    }

    DEBUG_ALLOCINTUIMESSAGE(dprintf("AllocIntuiMessage: Window 0x%lx Port 0x%lx Msg 0x%lx\n",
                                    window, window->WindowPort, msg));

    return msg;

    AROS_LIBFUNC_EXIT
}
