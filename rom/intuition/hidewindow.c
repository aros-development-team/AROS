/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include <proto/layers.h>
#include <intuition/gadgetclass.h>
#include "showhide.h"
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"

struct HideWindowActionMsg
{
    struct IntuiActionMsg  msg;
    struct Window         *window;
};

static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(BOOL, HideWindow,

/*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 141, Intuition)

/*  FUNCTION
        Make a window invisible.

    INPUTS
        window - The window to affect.

    RESULT
        TRUE if the operation was performed or queued successfully,
        FALSE otherwise.

    NOTES
        This function is source-compatible with AmigaOS v4.
        This function is also present in MorphOS v50, however
        considered private.

    EXAMPLE

    BUGS

    SEE ALSO
        ShowWindow()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct HideWindowActionMsg msg;

    DEBUG_HIDEWINDOW(dprintf("HideWindow: Window 0x%lx\n", (ULONG) window));
    SANITY_CHECKR(window, FALSE)

#ifdef CGXSHOWHIDESUPPORT
    if (window->Flags & WFLG_BACKDROP) return;
#endif

    msg.window = window;
    return DoASyncAction((APTR)int_hidewindow, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT
} /* HideWindow */


static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window  *window = msg->window;
#ifdef CGXSHOWHIDESUPPORT
    struct Library *CGXSystemBase;

    if (!window) return;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    if (window->Flags & WFLG_BACKDROP) return;

    if (((struct IntWindow *)(window))->specialflags & SPFLAG_NOICONIFY) return;

    if ((CGXSystemBase = OpenLibrary("cgxsystem.library", 0)))
    {
        CGXHideWindow(window);
        ((struct IntWindow *)(window))->specialflags |= SPFLAG_ICONIFIED;
        CloseLibrary(CGXSystemBase);
    }
#else
    struct Screen *screen;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    screen = window->WScreen;
    
    if (IsWindowVisible(window))
    {
        struct Requester *req;

        if (window == IntuitionBase->ActiveWindow)
        {
            struct IIHData *iihd =
                (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

            if (iihd->ActiveGadget &&
                !IS_SCREEN_GADGET(iihd->ActiveGadget) &&
                (iihd->GadgetInfo.gi_Window == window))
            {
                struct Gadget *gadget = iihd->ActiveGadget;

                switch (gadget->GadgetType & GTYP_GTYPEMASK)
                {
                    case GTYP_CUSTOMGADGET:
                    {
                        struct gpGoInactive gpgi;

                        gpgi.MethodID   = GM_GOINACTIVE;
                        gpgi.gpgi_GInfo = &iihd->GadgetInfo;
                        gpgi.gpgi_Abort = 1;

                        Locked_DoMethodA(window, gadget, (Msg)&gpgi, IntuitionBase);
                        break;
                    }

                    case GTYP_STRGADGET:
                        gadget->Flags &= ~GFLG_SELECTED;
                        RefreshStrGadget(gadget,
                                         iihd->GadgetInfo.gi_Window,
                                         iihd->GadgetInfo.gi_Requester,
                                         IntuitionBase);
                        break;

                    case GTYP_BOOLGADGET:
                        if (!(gadget->Activation & GACT_TOGGLESELECT))
                        {
                            BOOL inside;

                            inside = InsideGadget(iihd->GadgetInfo.gi_Screen,
                                                  iihd->GadgetInfo.gi_Window,
                                                  iihd->GadgetInfo.gi_Requester,
                                                  gadget,
                                                  iihd->GadgetInfo.gi_Screen->MouseX,
                                                  iihd->GadgetInfo.gi_Screen->MouseY);

                            if (inside)
                            {
                                gadget->Flags &= ~GFLG_SELECTED;
                                RefreshBoolGadgetState(gadget,
                                                       iihd->GadgetInfo.gi_Window,
                                                       iihd->GadgetInfo.gi_Requester,
                                                       IntuitionBase);
                            }
                        }
                        break;

                    case GTYP_PROPGADGET:
                        HandlePropSelectUp(gadget,
                                           iihd->GadgetInfo.gi_Window,
                                           NULL,
                                           IntuitionBase);
                        if (gadget->Activation & GACT_RELVERIFY)
                        {
                            ih_fire_intuimessage(iihd->GadgetInfo.gi_Window,
                                                 IDCMP_GADGETUP,
                                                 0,
                                                 gadget,
                                                 IntuitionBase);
                        }
                        break;
                }

                gadget->Activation &= ~GACT_ACTIVEGADGET;
                iihd->ActiveGadget = NULL;
            }

            ActivateWindow(NULL);
        }

        LOCK_REFRESH(screen);

        if (BLAYER(window))
        {
            ChangeLayerVisibility(BLAYER(window), FALSE);
        }
        ChangeLayerVisibility(WLAYER(window), FALSE);

        for (req = window->FirstRequest; req; req = req->OlderRequest)
        {
            ChangeLayerVisibility(req->ReqLayer, FALSE);
        }

        UNLOCK_REFRESH(screen);

        CheckLayers(screen, IntuitionBase);
    }
#endif
};
