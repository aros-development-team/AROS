/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

#ifdef SKINS
#   include "mosmisc.h"
#endif

struct EndRequestActionMsg
{
    struct IntuiActionMsg msg;
    struct Requester *requester;
    struct Window *window;
};

static VOID int_endrequest(struct EndRequestActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, EndRequest,

         /*  SYNOPSIS */
         AROS_LHA(struct Requester *, requester, A0),
         AROS_LHA(struct Window *   , window, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 20, Intuition)

/*  FUNCTION
    Remove a requester from the specified window.
    Other open requesters of this window stay alive.
 
    INPUTS
    requester - The requester to be deleted
    window - The window to which the requester belongs
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    InitRequester(), Request()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct EndRequestActionMsg msg;

    DEBUG_REQUEST(dprintf("EndRequest: req 0x%lx window 0x%lx\n", requester, window));

    SANITY_CHECK(window)
    SANITY_CHECK(requester)

    msg.requester = requester;
    msg.window = window;

    DoSyncAction((APTR)int_endrequest, &msg.msg, IntuitionBase);

    DEBUG_REQUEST(dprintf("EndRequest: removed succesfuly\n"));

    AROS_LIBFUNC_EXIT
} /* EndRequest */


static VOID int_endrequest(struct EndRequestActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window *window = msg->window;
    struct Requester *requester = msg->requester;
    struct Requester *p = window->FirstRequest;
    // struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    // int i;

    //jDc: intuition68k doesn't care (tested)
    //if (requester->Flags & REQACTIVE)
    {
        LOCKWINDOWLAYERS(window);
        
        if (p == requester)
        {
            window->FirstRequest = requester->OlderRequest;//unliked
        } else {
            while (p && (p->OlderRequest != requester))
                p = p->OlderRequest;

            if (p)
            {
                p->OlderRequest = requester->OlderRequest;//unlinked
            }
        }

        if (p)
        {
            struct Screen *screen = window->WScreen;

            --window->ReqCount;
            requester->Flags &= ~REQACTIVE;

            LOCK_REFRESH(screen);
            if (requester->ReqLayer) DeleteLayer(0, requester->ReqLayer);
            requester->OlderRequest = 0;
            requester->ReqLayer = 0; //sanity
            CheckLayers(screen, IntuitionBase);
            UNLOCK_REFRESH(screen);

            ih_fire_intuimessage(window,
                                 IDCMP_REQCLEAR,
                                 0,
                                 window,
                                 IntuitionBase);


        }

        UNLOCKWINDOWLAYERS(window);
    }
}
