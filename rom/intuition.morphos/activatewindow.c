/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/timer.h>
#include <intuition/gadgetclass.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"
#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "menus.h"
#include <intuition/pointerclass.h>

struct ActivateWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_activatewindow(struct ActivateWindowActionMsg *msg,
                               struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH1(void, ActivateWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 75, Intuition)

/*  FUNCTION
    Activates the specified window. The window gets the focus
    and all further input it sent to that window. If the window
    requested it, it will get a IDCMP_ACTIVEWINDOW message.

    INPUTS
    window - The window to activate

    RESULT
    None.

    NOTES
    If the user has an autopointer tool (sunmouse), the call will
    succeed, but the tool will deactivate the window right after
    this function has activated it. It is no good idea to try to
    prevent this by waiting for IDCMP_INACTIVEWINDOW and activating
    the window again since that will produce an anoying flicker and
    it will slow down the computer a lot.

    EXAMPLE

    BUGS

    SEE ALSO
    ModiyIDCMP(), OpenWindow(), CloseWindow()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct ActivateWindowActionMsg msg;

    DEBUG_ACTIVATEWINDOW(dprintf("ActivateWindow: Window 0x%lx\n",
                     window));

    msg.window = window;
    DoASyncAction((APTR)int_activatewindow, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* ActivateWindow */

/* This is called on the input.device's context */

static VOID int_activatewindow(struct ActivateWindowActionMsg *msg,
                               struct IntuitionBase *IntuitionBase)
{
    struct IIHData  	*iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    /* On the Amiga ActivateWindow is delayed if there
       is an active gadget (altough this does not seem
       to be true for string gadgets). We just ignore
       ActivateWindow in such a case. */

    struct Window   	*window = msg->window;
    ULONG   	     	 lock;
    struct Window   	*oldactive;
    Object  	    	*pointer = NULL;
    struct IntScreen 	*scr = NULL;
    struct InputEvent 	*ie;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    if ((!iihdata->ActiveGadget) ||
    	(iihdata->ActiveGadget && ((iihdata->ActiveGadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SDEPTH)))
    {

        lock = LockIBase(0UL);

        oldactive = IntuitionBase->ActiveWindow;
        IntuitionBase->ActiveWindow = window;

        DEBUG_ACTIVATEWINDOW(dprintf("IntActivateWindow: Window 0x%lx OldActive 0x%lx\n",
                         window, oldactive));

        if (oldactive != window)
        {
            GetPrivIBase(IntuitionBase)->PointerDelay = 0;

            if (oldactive)
            {
                ih_fire_intuimessage(oldactive,
                             IDCMP_INACTIVEWINDOW,
                             0,
                             oldactive,
                             IntuitionBase);
			     
                scr = GetPrivScreen(oldactive->WScreen);
                scr->Screen.Title = scr->Screen.DefaultTitle;
            }

            if (window)
            {
                /*
                    Tasks are allowed to modify window->Flags, for example
                     set/clear WFLG_RMBTRAP. It is not certain that every 
                     compiler on every machine produces an atomic instruction.
                */
                #warning check that window->Flags is atomically set everywhere!
                AROS_ATOMIC_OR(window->Flags, WFLG_WINDOWACTIVE);
                
                pointer = IW(window)->pointer;
                if (IW(window)->busy)
                    pointer = GetPrivIBase(IntuitionBase)->BusyPointer;

                scr = GetPrivScreen(window->WScreen);
                if (window->ScreenTitle)
                    scr->Screen.Title = window->ScreenTitle;
            }

            /* now set the ActiveScreen! */
            if (scr)
            {
                struct Screen *old;
                struct Window *win;

                old = IntuitionBase->ActiveScreen;

                if (old && (old != (struct Screen *)scr))
                {

                    /* this avoids mouse "jump" effect */
                    scr->Screen.MouseX = old->MouseX;
                    scr->Screen.MouseY = old->MouseY;

                    win = scr->Screen.FirstWindow;

                    while (win)
                    {
                        UpdateMouseCoords(win);
                        win = win->NextWindow;
                    }
                }

                IntuitionBase->ActiveScreen = (struct Screen *)scr;

            }

            if (scr)
            {
                struct SharedPointer *shared_pointer;

                if (!pointer)
                    pointer = GetPrivIBase(IntuitionBase)->DefaultPointer;

                GetAttr(POINTERA_SharedPointer, pointer, (IPTR *)&shared_pointer);

                DEBUG_POINTER(dprintf("ActivateWindow: scr 0x%lx pointer 0x%lx sprite 0x%lx\n",
                              scr, pointer, shared_pointer->sprite));

                if (ChangeExtSpriteA(&scr->Screen.ViewPort,
                             scr->Pointer->sprite, shared_pointer->sprite, NULL))
                {
                    ObtainSharedPointer(shared_pointer, IntuitionBase);
                    ReleaseSharedPointer(scr->Pointer, IntuitionBase);
                    scr->Pointer = shared_pointer;
                    if (window)
                    {
                        window->XOffset = shared_pointer->xoffset;
                        window->YOffset = shared_pointer->yoffset;
                    }

                    //jDc: fix some weird refresh pbs
                    SetPointerPos(scr->Screen.MouseX, scr->Screen.MouseY);
                }
                else
                {
                    DEBUG_POINTER(dprintf("ActivateWindow: can't set pointer.\n"));
                }
            }
        }

        UnlockIBase(lock);

        if (oldactive && oldactive != window)
        {
            AROS_ATOMIC_AND(oldactive->Flags, ~WFLG_WINDOWACTIVE);
         
            int_refreshwindowframe(oldactive, REFRESHGAD_BORDER, 0, IntuitionBase);
            if (!window || oldactive->WScreen != window->WScreen)
                RenderScreenBar(oldactive->WScreen, FALSE, IntuitionBase);
        }


        if (window && oldactive != window)
        {
            int_refreshwindowframe(window, REFRESHGAD_BORDER, 0, IntuitionBase);
            RenderScreenBar(window->WScreen, FALSE, IntuitionBase);
        }

#ifdef TIMEVALWINDOWACTIVATION
        if (window)
        {
            GetSysTime(&IW(window)->activationtime);
        }
#endif

        if ((ie = AllocInputEvent(iihdata)))
        {
            ie->ie_Class = IECLASS_EVENT;
            ie->ie_Code = IECODE_NEWACTIVE;
            ie->ie_EventAddress = window;
            CurrentTime(&ie->ie_TimeStamp.tv_secs, &ie->ie_TimeStamp.tv_micro);
        }

        if (window)
        {
            ih_fire_intuimessage(window,
                         IDCMP_ACTIVEWINDOW,
                         0,
                         window,
                         IntuitionBase);
        }
    }
    else
    {
        int_refreshwindowframe(window, REFRESHGAD_BORDER, 0, IntuitionBase);
    }

#ifdef __MORPHOS__
    Forbid();
    if (window && window->UserPort)
    {
        struct Task *apptask = window->UserPort->mp_SigTask;
	
        if (apptask && (!apptask->tc_SigWait) && (apptask->tc_State == TS_WAIT))
        {
            //task is DEAD!
            //give some visual feedback to the user
	    
            IW(window)->specialflags |= SPFLAG_IAMDEAD;

            int_refreshwindowframe(window,REFRESHGAD_TOPBORDER,0,IntuitionBase);

        }
	else
	{
            IW(window)->specialflags &= ~SPFLAG_IAMDEAD;
        }
    }
    Permit();
#endif

}
