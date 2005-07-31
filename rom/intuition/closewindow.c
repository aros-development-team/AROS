/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/timer.h>
#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"
//#include "segtracker.h"
#include <intuition/gadgetclass.h>

#ifndef DEBUG_CloseWindow
#   define DEBUG_CloseWindow 1
#endif
#undef DEBUG
#if DEBUG_CloseWindow
#   define DEBUG 1
#endif
#   include <aros/debug.h>

/******************************************************************************/

#define MUST_UNLOCK_SCREEN(window,screen) (((GetPrivScreen(screen)->pubScrNode != NULL) && \
    	    	    	    	    	   (window->MoreFlags & WMFLG_DO_UNLOCKPUBSCREEN)) ? TRUE : FALSE)

struct CloseWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

VOID int_closewindow(struct CloseWindowActionMsg *msg,
                     struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH1(void, CloseWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 12, Intuition)

/*  FUNCTION
    Closes a window. Depending on the display, this might not happen
    at the time when this function returns, but you must not use
    the window pointer after this function has been called.

    INPUTS
    window - The window to close

    RESULT
    None.

    NOTES
    The window might not have been disappeared when this function returns.

    EXAMPLE

    BUGS

    SEE ALSO
    OpenWindow(), OpenWindowTags()

    INTERNALS

    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct CloseWindowActionMsg  msg;
    struct IIHData          	*iihd;
    struct MsgPort          	*userport;
#if USE_IDCMPUPDATE_MESSAGECACHE
    struct IntuiMessage     	*messagecache;
#endif
    struct Screen           	*screen;
    BOOL            	    	 do_unlockscreen;

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: Window 0x%lx\n", window));

    D(bug("CloseWindow (%p)\n", window));

    if ( window == NULL )
    {
        ReturnVoid ("CloseWindow");
    }

    iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    screen = window->WScreen;
    do_unlockscreen = MUST_UNLOCK_SCREEN(window, screen);

#ifndef __MORPHOS__
    /* We take a very simple approach to avoid race conditions with the
       intuition input handler running one input.device 's task:
       We just send it a msg about closing the window
    */
    if (HAS_CHILDREN(window))
    {
        struct Window * cw = window->firstchild;
        /*
         * First close all its children, we could also return
         * a failure value which newer apps that use child windows
         * have to look at.
         */
        while (cw)
        {
            struct Window * _cw;
	    
            _cw = cw->nextchild;
            CloseWindow(cw);
            cw = _cw;
        }

    }

    if (IS_CHILD(window))
    {
        /*
         * Unlink the window from its parent or
         * out of the list of child windows.
         */
        if (window->parent->firstchild == window)
            window->parent->firstchild = window->nextchild;
        else
            window->prevchild->nextchild = window->nextchild;

        if (window->nextchild)
            window->nextchild->prevchild = window->prevchild;

    }
#endif

    /* We must save this here, because after we have returned from
       the Wait() the window is gone  */
    userport = window->UserPort;

#if USE_IDCMPUPDATE_MESSAGECACHE
    messagecache = IW(window)->messagecache;
#endif

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: Userport 0x%lx\n", userport));

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: DoSyncAction\n"));

#ifdef USEWINDOWLOCK
    /* wait until other open/close && move/size && menus actions are finished */
    if (!(FindTask(0) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask))
    {
        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
    }
#endif

    msg.window = window;
    DoSyncAction((APTR)int_closewindow, &msg.msg, IntuitionBase);

#ifdef USEWINDOWLOCK
    if (!(FindTask(0) == ((struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data)->InputDeviceTask))
    {
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
    }
#endif

    if (do_unlockscreen) UnlockPubScreen(NULL, screen);

    /* As of now intuition has removed us from th list of
       windows, and we will recieve no more messages
    */
#if USE_IDCMPUPDATE_MESSAGECACHE
    if (messagecache)
    {
        messagecache->IDCMPWindow = 0;//zero or we'll trash mem in inputhandler!
        messagecache->Code = 0;
        messagecache->Qualifier = 0;
        ReplyMsg(&messagecache->ExecMessage);
    }
#endif

    if (userport)
    {
        struct IntuiMessage *im;

        DEBUG_CLOSEWINDOW(dprintf("CloseWindow: Reply UserPort Msgs\n"));
        while ((im = (struct IntuiMessage *) GetMsg (userport)))
        {
	    im->IDCMPWindow = 0;
            im->Code = 0;
            im->Qualifier = 0;
            ReplyMsg ((struct Message *)im);
        }

        /* Delete message port */
        DeleteMsgPort (userport);
    }

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: done\n"));
    ReturnVoid ("CloseWindow");
    AROS_LIBFUNC_EXIT
} /* CloseWindow */

/******************************************************************************/


/* This is called from the intuition input handler */
VOID int_closewindow(struct CloseWindowActionMsg *msg,
                     struct IntuitionBase *IntuitionBase)
{
    /* Free everything except the applications messageport */
    struct Window   *window, *win2;
    struct Screen   *screen;
    struct MsgPort  *userport;
    struct IIHData  *iihd;
    ULONG            lock;
    BOOL             do_unlockscreen;

    D(bug("CloseWindow (%p)\n", window));

    window = msg->window;
    
    RemoveResourceFromList(window, RESOURCE_WINDOW, IntuitionBase);

    iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    /* if there is an active gadget in the window being closed, then make
       it inactive */
    if (iihd->ActiveGadget &&
        !IS_SCREEN_GADGET(iihd->ActiveGadget) &&
        (iihd->GadgetInfo.gi_Window == window))
    {
        struct Gadget *gadget = iihd->ActiveGadget;

        switch(gadget->GadgetType & GTYP_GTYPEMASK)
        {
            case GTYP_CUSTOMGADGET:
        	{
                    struct gpGoInactive gpgi;

                    gpgi.MethodID   = GM_GOINACTIVE;
                    gpgi.gpgi_GInfo = &iihd->GadgetInfo;
                    gpgi.gpgi_Abort = 1;

                    Locked_DoMethodA(window, gadget, (Msg)&gpgi, IntuitionBase);

                    if (iihd->ActiveSysGadget)
                    {
                	gadget = iihd->ActiveSysGadget;
                	iihd->ActiveSysGadget = NULL;

                	if (IS_BOOPSI_GADGET(gadget))
                	{
                            Locked_DoMethodA(window, gadget, (Msg)&gpgi, IntuitionBase);
                	}
                    }
        	}
        	break;

            case GTYP_STRGADGET:
            case GTYP_BOOLGADGET:
        	gadget->Flags &= ~GFLG_SELECTED;
        	break;
        }

        gadget->Activation &= ~GACT_ACTIVEGADGET;

        iihd->ActiveGadget = NULL;
    }

    /* Need this in case of a window created under the input.device task context */
    screen = window->WScreen;
    userport = window->UserPort;
    do_unlockscreen = MUST_UNLOCK_SCREEN(window, screen);

    /* Check if there are still some requesters */
    //jDc: do NOT use this! if there is a requester without ACTIVE flag set this routine will buysloop!
    //jDc: moved this stuff down to layer dispose stuff!
/*    while (window->FirstRequest)
        EndRequest(window->FirstRequest, window);*/

    lock = LockIBase (0);

    if (window == IntuitionBase->ActiveWindow)
        IntuitionBase->ActiveWindow = NULL;
    if (window == iihd->NewActWindow) iihd->NewActWindow = NULL;

    /* Remove window from the parent/descendant chain and find next active window
    **
    **
    ** before:  parent win xyz
    **              \
    **           \
    **       deadwindow window
    **          /
    **         /
    **        /
    **      descendant win abc
    **
    ** after: parent win xyz
    **            |
    **            |
    **            |
    **        descendant win abc
    **
    */

    if (window->Descendant)
        window->Descendant->Parent = window->Parent;
    if (window->Parent)
        window->Parent->Descendant = window->Descendant;
    
    /* Was this the active window? */
    if (!IntuitionBase->ActiveWindow)
    {
        /* If so, we need to find out which window to make
	   active now. We first check whether we have a "parent",
	   which is a window that was open before the one we're closing. */
        if (window->Parent)
            ActivateWindow (window->Parent);
	else
	/* Otherwise, we find out which was the latest one, and activate it.
	   It's debatable whether this is the best policy, but this is how
	   AmigaOS(TM) does it.  */
	if ((win2 = window->Descendant))
	{
	    for (;win2->Descendant; win2 = win2->Descendant);
                ActivateWindow (win2);
	}
    }       

    /* Make sure the Screen's window list is still valid */

    if (window == window->WScreen->FirstWindow)
    {
        window->WScreen->FirstWindow = window->NextWindow;
    }
    else if ((win2 = window->WScreen->FirstWindow))
    {
        while (win2->NextWindow)
        {
            if (win2->NextWindow == window)
            {
                win2->NextWindow = window->NextWindow;
                break;
            }
            win2 = win2->NextWindow;
        }
    }

    UnlockIBase (lock);

#ifdef TIMEVALWINDOWACTIVATION
    if (window->WScreen->FirstWindow && !IntuitionBase->ActiveWindow)
    {
        struct Window *neww = 0,*scanw = 0;

        for (scanw = window->WScreen->FirstWindow; scanw ; scanw = scanw->NextWindow)
        {
            if (neww)
            {
                if ((IW(scanw)->activationtime.tv_secs > IW(neww)->activationtime.tv_secs) ||
                    ((IW(scanw)->activationtime.tv_secs == IW(neww)->activationtime.tv_secs) && (IW(scanw)->activationtime.tv_micro > IW(neww)->activationtime.tv_micro)))
                {
                    neww = scanw;
                }
            }

            if (!neww) neww = scanw;
        }

        if (neww) ActivateWindow(neww);

    }
#endif

    /* Free resources */

    LOCK_REFRESH(screen);

    /* IFont may be NULL if we are called from an OpenWindow failure */
    if (window->IFont)
        CloseFont (window->IFont);

    #ifdef DAMAGECACHE
    if (IW(window)->trashregion) DisposeRegion(IW(window)->trashregion);
    #endif

    if (window->FirstRequest)
    {
        struct Requester *r = window->FirstRequest;
	
        while (r)
        {
            if (r->ReqLayer) DeleteLayer(0,r->ReqLayer);
            r->ReqLayer = 0;
            r = r->OlderRequest;
        }
    }

    // remove transparency!
#ifdef SKINS
    if (WLAYER(window))
    {
        InstallTransparentRegionHook(WLAYER(window),NULL);
        InstallTransparentRegion(WLAYER(window),NULL);
    }

    if (((struct IntWindow *)(window))->transpregion) DisposeRegion(((struct IntWindow *)(window))->transpregion);
#endif

    /* Let the driver clean up. Driver wil dealloc window's rastport */
    intui_CloseWindow (window, IntuitionBase);

    /* jDc: trash the screen pointer to avoid unnecessary checks in WindowValid() and
       memory corruption */
    window->WScreen = (struct Screen *)0xC0DEBAD0;

    /* Free memory for the window */
    FreeMem (window, sizeof(struct IntWindow));

    CheckLayers(screen, IntuitionBase);

    UNLOCK_REFRESH(screen);
} /* int_closewindow */


/**********************************************************************************/


void intui_CloseWindow (struct Window * w,
                        struct IntuitionBase * IntuitionBase)
{
    KillWinSysGadgets(w, IntuitionBase);

    if (0 == (w->Flags & WFLG_GIMMEZEROZERO))
    {
        /* not a GZZ window */
        if (WLAYER(w))
            DeleteLayer(0, WLAYER(w));
        DeinitRastPort(w->BorderRPort);
        FreeMem(w->BorderRPort, sizeof(struct RastPort));
    }
    else
    {
        /* a GZZ window */
        /* delete inner window */
        if (NULL != WLAYER(w))
            DeleteLayer(0, WLAYER(w));

        /* delete outer window */
        if (NULL != BLAYER(w))
            DeleteLayer(0, BLAYER(w));
    }

    if (IW(w)->free_pointer)
        DisposeObject(IW(w)->pointer);

}
