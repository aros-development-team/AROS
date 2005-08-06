/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Close a screen.
*/

#include <graphics/videocontrol.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

#ifdef SKINS
#   include "intuition_customize.h"
#endif

#ifndef DEBUG_CloseScreen
#define DEBUG_CloseScreen 0
#endif
#undef DEBUG
#if DEBUG_CloseScreen
#define DEBUG 1
#endif
#include <aros/debug.h>

struct CloseScreenActionMsg
{
    struct IntuiActionMsg msg;
    struct Screen   	 *Screen;
};

static VOID int_closescreen(struct CloseScreenActionMsg *msg,
                            struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <intuition/screens.h>
#include <proto/intuition.h>

AROS_LH1(BOOL, CloseScreen,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, screen, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 11, Intuition)

/*  FUNCTION
 
    Release all resources held by a screen and close it down visually.
 
    INPUTS
 
    screen  --  pointer to the screen to be closed
 
    RESULT
 
    TRUE if the screen is successfully closed, FALSE if there were still
    windows left on the screen (which means the screen is not closed).
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct CloseScreenActionMsg msg;

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: Screen 0x%lx\n", screen));

    D(bug("CloseScreen (%p)\n", screen));

    if ( screen == NULL )
    {
        ReturnBool("CloseScreen",TRUE);
    }

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Notify that the screen is going to close */
    sn_DoNotify(SCREENNOTIFY_TYPE_CLOSESCREEN, screen, GetPrivIBase(IntuitionBase)->ScreenNotifyBase);
#endif

    /* there's a second check below for public screens */
    if (screen->FirstWindow)
    {
        D(bug("CloseScreen: fail, window still opened\n"));
        ReturnBool("CloseScreen",FALSE);
    }

#ifdef USEWINDOWLOCK
    /* let's wait for user to finish window drag/size actions to avoid
    deadlocks and not break user's input */
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#endif

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockPubScreenList\n"));
    LockPubScreenList();
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockPubScreenList done\n"));

    msg.Screen = screen;
    DoSyncAction((APTR)int_closescreen,&msg.msg,IntuitionBase);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockPubScreenList\n"));
    UnlockPubScreenList();
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockPubScreenList done\n"));

    if (GetPrivScreen(screen)->pubScrNode != NULL)
    {
#ifdef USEWINDOWLOCK
        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#endif
        DEBUG_CLOSESCREEN(dprintf("CloseScreen: failed\n"));
        //something went wrong! int_closescreen is supposed to clear pubScrNode ptr!
        return FALSE;
    }

    /* kill screen bar */
    KillScreenBar(screen, IntuitionBase);

    /* kill depth gadget */
    if (((struct IntScreen *)screen)->depthgadget)
    {
        Object *im = (Object*)((struct Gadget *)(((struct IntScreen *)screen)->depthgadget))->GadgetRender;

        DisposeObject(im);
        DisposeObject(((struct IntScreen *)screen)->depthgadget);
    }

#ifdef __MORPHOS__
    RethinkDisplay();

    FreeVPortCopLists(&screen->ViewPort);

    {
        struct TagItem tags[2];

        tags[0].ti_Tag = VTAG_ATTACH_CM_GET;
        tags[0].ti_Data = 0;
        tags[1].ti_Tag = VTAG_END_CM;

        if (VideoControl(screen->ViewPort.ColorMap, tags))
        {
            GfxFree((APTR)tags[0].ti_Data);
        }
    }
#else
/* !!! Setting a new front bitmap MUST be done before freeing the old one */
    if (NULL != IntuitionBase->FirstScreen)
    {
        /* We MUST pas FALSE in the "copyback" parameter
        since the old screen bitmap has been deleted
        */
        SetFrontBitMap(IntuitionBase->FirstScreen->RastPort.BitMap, FALSE);

    }
    else
    {
        SetFrontBitMap(NULL, FALSE);
    }
#endif

#ifdef USEWINDOWLOCK
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->WindowLock);
#endif

    /* Free the RasInfo of the viewport */
    FreeMem(screen->ViewPort.RasInfo, sizeof (struct RasInfo));

    /* Uninit the layerinfo */

    ThinLayerInfo(&screen->LayerInfo);

    /* Free the screen's bitmap */

    if (GetPrivScreen(screen)->AllocatedBitmap)
        FreeBitMap(GetPrivScreen(screen)->AllocatedBitmap);
    screen->RastPort.BitMap = NULL;

    /* Free the RastPort's contents */
    DeinitRastPort(&screen->RastPort);

    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_ScrDecorObj);
    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_WinDecorObj);
    
#ifdef SKINS
    if (((struct IntScreen *)screen)->DInfo.dri_Customize)
    {
        /* Free the skin */
        DisposeObject(((struct IntScreen *)screen)->DInfo.dri_Customize->submenu);
        DisposeObject(((struct IntScreen *)screen)->DInfo.dri_Customize->menutoggle);
        int_SkinAction(SKA_FreeSkin,(ULONG*)&((struct IntScreen *)(screen))->DInfo,(struct Screen *)screen,IntuitionBase);
        int_FreeTitlebarBuffer(((struct IntScreen *)(screen)),IntuitionBase);
        FreeMem(((struct IntScreen *)screen)->DInfo.dri_Customize,sizeof (struct IntuitionCustomize));
    }
#endif

    DisposeObject(((struct IntScreen *)screen)->DInfo.dri.dri_CheckMark);
    DisposeObject(((struct IntScreen *)screen)->DInfo.dri.dri_AmigaKey);

    /* Close the font */
    CloseFont(((struct IntScreen *)screen)->DInfo.dri.dri_Font);

    /* Free the ColorMap */
    FreeColorMap(screen->ViewPort.ColorMap);

    /* Free the sprite */
    ReleaseSharedPointer(((struct IntScreen *)screen)->Pointer, IntuitionBase);

    /* Free the memory */
    FreeMem(screen, sizeof (struct IntScreen));

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: ok\n"));

    ReturnBool("CloseScreen",TRUE);
 
    AROS_LIBFUNC_EXIT
} /* CloseScreen */

static VOID int_closescreen(struct CloseScreenActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    struct Screen *parent,*screen = msg->Screen;
    struct Window *win = 0;
    ULONG   	   lock;

    /* If this is a public screen, free related information if there are
       no windows left on the screen */
    if (GetPrivScreen(screen)->pubScrNode != NULL)
    {
        if(GetPrivScreen(screen)->pubScrNode->psn_VisitorCount || screen->FirstWindow)
        {
            DEBUG_CLOSESCREEN(dprintf("CloseScreen: failed\n"));
            return;
        }

        Remove((struct Node *)GetPrivScreen(screen)->pubScrNode);

        if(GetPrivScreen(screen)->pubScrNode->psn_Node.ln_Name != NULL)
            FreeVec(GetPrivScreen(screen)->pubScrNode->psn_Node.ln_Name);

        FreeMem(GetPrivScreen(screen)->pubScrNode,
                sizeof(struct PubScreenNode));

        GetPrivScreen(screen)->pubScrNode = 0;
    }

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockIBase\n"));

    lock = LockIBase(0);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockIBase done\n"));

    /* Trick: Since NextScreen is the first field of the structure,
    we can use the pointer in the IntuitionBase as a screen with
    the structure-size of one pointer */
    parent = (struct Screen *)&(IntuitionBase->FirstScreen);

    /* For all screens... */
    while (parent->NextScreen)
    {
        /* If the screen to close is the next screen... */
        if (parent->NextScreen == screen)
        {

            /* Unlink it */
            parent->NextScreen = screen->NextScreen;

            /* Check ActiveScreen */
            if (IntuitionBase->ActiveScreen == screen)
            {
                if (screen->NextScreen)
                    IntuitionBase->ActiveScreen = screen->NextScreen;
                else if (IntuitionBase->FirstScreen)
                    IntuitionBase->ActiveScreen = parent;
                else
                    IntuitionBase->ActiveScreen = NULL;
            }

            /* now let's set the default pub screen */
            if (GetPrivIBase(IntuitionBase)->DefaultPubScreen == screen)
            {
                struct Screen *scr;
                BOOL nexttry = TRUE;

                GetPrivIBase(IntuitionBase)->DefaultPubScreen = NULL;
                scr = IntuitionBase->FirstScreen;

                if (scr && GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN && GetPrivScreen(scr)->pubScrNode && (scr->Flags & (PUBLICSCREEN | WBENCHSCREEN)))
                {
                    GetPrivIBase(IntuitionBase)->DefaultPubScreen = scr;
                }
            }

            DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase\n"));
            UnlockIBase(lock);
            DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase done\n"));

#ifdef TIMEVALWINDOWACTIVATION
            if (IntuitionBase->FirstScreen && ((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENACTIVATION) || !IntuitionBase->ActiveWindow))
            {
                struct Window *scanw = 0;

                for (scanw = IntuitionBase->FirstScreen->FirstWindow; scanw ; scanw = scanw->NextWindow)
                {
                    if (win)
                    {
                        if ((IW(scanw)->activationtime.tv_secs > IW(win)->activationtime.tv_secs) ||
                            ((IW(scanw)->activationtime.tv_secs == IW(win)->activationtime.tv_secs) && (IW(scanw)->activationtime.tv_micro > IW(win)->activationtime.tv_micro)))
                        {
                            win = scanw;
                        }
                    }

                    if (!win) win = scanw;
                }

                if (!win) win = IntuitionBase->FirstScreen->FirstWindow;
                if (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_SCREENACTIVATION)
                    if (IntuitionBase->ActiveWindow && IntuitionBase->ActiveWindow->WScreen == IntuitionBase->FirstScreen) win = NULL;
                if (win) ActivateWindow(win);
            }
#endif
            return;
        }
        parent = parent->NextScreen;
    }

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase\n"));
    UnlockIBase(lock);
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase done\n"));

    return;
}
