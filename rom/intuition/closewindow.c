/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
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
    struct Window           *window;
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
        The window might not have disappeared when this function returns.

    EXAMPLE

    BUGS

    SEE ALSO
        OpenWindow(), OpenWindowTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct CloseWindowActionMsg  msg;
    struct MsgPort              *userport;
#if USE_IDCMPUPDATE_MESSAGECACHE
    struct IntuiMessage         *messagecache;
#endif
    struct Screen               *screen;
    BOOL                         do_unlockscreen;

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: Window 0x%lx\n", window));

    D(bug("CloseWindow (%p)\n", window));

    if ( window == NULL )
    {
        ReturnVoid ("CloseWindow");
    }

    FireScreenNotifyMessage((IPTR) window, SNOTIFY_BEFORE_CLOSEWINDOW, IntuitionBase);


    screen = window->WScreen;
    do_unlockscreen = MUST_UNLOCK_SCREEN(window, screen);

#ifndef __MORPHOS__
    /* We take a very simple approach to avoid race conditions with the
       intuition input handler running on input.device's task:
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

    /* As of now intuition has removed us from the list of
       windows, and we will receive no more messages
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

    FireScreenNotifyMessage((IPTR) window, SNOTIFY_AFTER_CLOSEWINDOW, IntuitionBase);

    DEBUG_CLOSEWINDOW(dprintf("CloseWindow: done\n"));
    ReturnVoid ("CloseWindow");
    AROS_LIBFUNC_EXIT
} /* CloseWindow */

/******************************************************************************/


/* This is called from the intuition input handler */
VOID int_closewindow(struct CloseWindowActionMsg *msg,
                     struct IntuitionBase *IntuitionBase)
{
    /* Free everything except the application's messageport */
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window   **winprev_ptr, *wincur;
    struct Screen   *screen;
    struct IIHData  *iihd;
    ULONG            lock;

    D(bug("CloseWindow (%p)\n", msg->window));

    RemoveResourceFromList(msg->window, RESOURCE_WINDOW, IntuitionBase);

    iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    /* if there is an active gadget in the msg->window being closed, then make
       it inactive */
    if (iihd->ActiveGadget &&
        !IS_SCREEN_GADGET(iihd->ActiveGadget) &&
        (iihd->GadgetInfo.gi_Window == msg->window))
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

                    Locked_DoMethodA(msg->window, gadget, (Msg)&gpgi, IntuitionBase);

                    if (iihd->ActiveSysGadget)
                    {
                        gadget = iihd->ActiveSysGadget;
                        iihd->ActiveSysGadget = NULL;

                        if (IS_BOOPSI_GADGET(gadget))
                        {
                            Locked_DoMethodA(msg->window, gadget, (Msg)&gpgi, IntuitionBase);
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

    /* Need this in case of a msg->window created under the input.device task context */
    screen = msg->window->WScreen;

    /* Check if there are still some requesters */
    //jDc: do NOT use this! if there is a requester without ACTIVE flag set this routine will buysloop!
    //jDc: moved this stuff down to layer dispose stuff!
/*    while (msg->window->FirstRequest)
        EndRequest(msg->window->FirstRequest, msg->window);*/

    lock = LockIBase (0);

    if (msg->window == IntuitionBase->ActiveWindow)
        IntuitionBase->ActiveWindow = NULL;
    if (msg->window == iihd->NewActWindow) iihd->NewActWindow = NULL;

    /* Remove msg->window from the parent/descendant chain and find next active msg->window
    **
    **
    ** before:  parent win xyz
    **              \
    **           \
    **       deadwindow msg->window
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

    if (msg->window->Descendant)
        msg->window->Descendant->Parent = msg->window->Parent;
    if (msg->window->Parent)
        msg->window->Parent->Descendant = msg->window->Descendant;
    
    /* Was this the active msg->window? */
    if (!IntuitionBase->ActiveWindow) {
        /* If so, we need to find out which msg->window to make
           active now. We first check whether we have a "parent",
           which is a msg->window that was open before the one we're closing. */
        if (msg->window->Parent)
            ActivateWindow (msg->window->Parent);
        else {
            /* Otherwise, we find out which was the latest one, and activate it.
               It's debatable whether this is the best policy, but this is how
               AmigaOS(TM) does it.  */
            if ((wincur = msg->window->Descendant)) {
                for (;wincur->Descendant; wincur = wincur->Descendant)
                    ;
                ActivateWindow(wincur);
            }
        }
    }

    /* Make sure the Screen's msg->window list is still valid */
    winprev_ptr = &msg->window->WScreen->FirstWindow;
    wincur = *winprev_ptr;
    while (wincur) {
        if (wincur == msg->window) {
            *winprev_ptr = wincur->NextWindow;
            break;
        }
        winprev_ptr = &wincur->NextWindow;
        wincur = wincur->NextWindow;
    }

    UnlockIBase (lock);

#ifdef TIMEVALWINDOWACTIVATION
    if (msg->window->WScreen->FirstWindow && !IntuitionBase->ActiveWindow)
    {
        struct Window *neww = 0,*scanw = 0;

        for (scanw = msg->window->WScreen->FirstWindow; scanw ; scanw = scanw->NextWindow)
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
    if (msg->window->IFont)
        CloseFont (msg->window->IFont);

    #ifdef DAMAGECACHE
    if (IW(msg->window)->trashregion) DisposeRegion(IW(msg->window)->trashregion);
    #endif

    if (msg->window->FirstRequest)
    {
        struct Requester *r = msg->window->FirstRequest;
        
        while (r)
        {
            if (r->ReqLayer) DeleteLayer(0,r->ReqLayer);
            r->ReqLayer = 0;
            r = r->OlderRequest;
        }
    }

    // remove transparency!
#ifdef SKINS
    if (WLAYER(msg->window))
    {
        InstallTransparentRegionHook(WLAYER(msg->window),NULL);
        InstallTransparentRegion(WLAYER(msg->window),NULL);
    }

    if (((struct IntWindow *)(msg->window))->transpregion) DisposeRegion(((struct IntWindow *)(msg->window))->transpregion);
#endif

    /* Let the driver clean up. Driver will dealloc msg->window's rastport */
    intui_CloseWindow (msg->window, IntuitionBase);

    /* jDc: trash the screen pointer to avoid unnecessary checks in WindowValid() and
       memory corruption */
    msg->window->WScreen = (struct Screen *)0xC0DEBAD0;

    /* If the screen was a  public screen, check if we were the last window */
    if (IS(screen)->pubScrNode != NULL)
    {
        if (!(IS(screen)->pubScrNode->psn_VisitorCount && screen->FirstWindow)) {
            
        }
    }

    /* Remove the Window Outline Shape */
    if (((struct IntWindow *)msg->window)->OutlineShape) DisposeRegion(((struct IntWindow *)msg->window)->OutlineShape);
    /* Push ExitScreen Message to the Screensdecoration Class */
    struct wdpExitWindow       wemsg;

    wemsg.MethodID             = WDM_EXITWINDOW;
    wemsg.wdp_UserBuffer       = ((struct IntWindow *)msg->window)->DecorUserBuffer;
    wemsg.wdp_TrueColor        = (((struct IntScreen *)screen)->DInfo.dri_Flags & DRIF_DIRECTCOLOR) ? TRUE : FALSE;

    DoMethodA(((struct IntScreen *)(screen))->WinDecorObj, (Msg)&wemsg);

    if (((struct IntWindow *)msg->window)->DecorUserBuffer)
    {
        FreeMem((APTR)((struct IntWindow *)msg->window)->DecorUserBuffer, ((struct IntWindow *)msg->window)->DecorUserBufferSize);
    }


    /* Free memory for the msg->window */
    FreeMem (msg->window, sizeof(struct IntWindow));

    CheckLayers(screen, IntuitionBase);

    UNLOCK_REFRESH(screen);
} /* int_closewindow */


/**********************************************************************************/


void intui_CloseWindow (struct Window * w,
                        struct IntuitionBase * IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
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
