/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2013, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Close a screen.
*/

/*
 * Things added by AROS, which needs to be kept when merging with newer MorphOS releases:
 *
 * 1. Explicit library bases
 * 2. FireScreenNotifyMessage() calls
 * 3. RemoveResourceFromList() call
 * 4. int_ExitDecorator() call
 * 5. Other placed marked by 'AROS:' in comments.
 * 6. Check #ifdef's. Some of them were rearranged or completely deleted.
 *    We reuse MorphOS skin code where appropriate.
 */

#include <graphics/videocontrol.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include "intuition_intern.h"
#include "intuition_customize.h"
#include "inputhandler_actions.h"

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
    struct Screen *Screen;
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
        screen - pointer to the screen to be closed

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

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct CloseScreenActionMsg msg;
//      ULONG lock;

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: Screen 0x%lx\n", (ULONG)screen));

    D(bug("CloseScreen (%p)\n", screen));

    if ( screen == NULL )
    {
        ReturnBool("CloseScreen",TRUE);
    }

#ifdef INTUITION_NOTIFY_SUPPORT
    /* Notify that the screen is going to close */
    sn_DoNotify(SCREENNOTIFY_TYPE_CLOSESCREEN, screen, IBase->ScreenNotifyBase);
#endif
    if (screen != GetPrivIBase(IntuitionBase)->WorkBench)
        FireScreenNotifyMessage((IPTR) screen, SNOTIFY_BEFORE_CLOSESCREEN, IntuitionBase);

    /* there's a second check below for public screens */
    if (screen->FirstWindow)
    {
        D(bug("CloseScreen: fail, window still opened\n"));
        ReturnBool("CloseScreen",FALSE);
    }

#ifdef USEWINDOWLOCK
    /* let's wait for user to finish window drag/size actions to avoid
    deadlocks and not break user's input */
    if (IS(screen)->WindowLock)
    {
        struct Screen *ccs;
        BOOL found = FALSE;
    
        LOCKWINDOW;

        /* !moron check here! */
        for (ccs = IntuitionBase->FirstScreen; ccs; ccs = ccs->NextScreen)
        {
            if (ccs == screen) found = TRUE;
        }

        if (!found)
        {
            struct Task *caller = FindTask(NULL);
            UNLOCKWINDOW;

            dprintf("CloseScreen: task %p (%s) attempted a bogus CloseScreen(%p) !\n",
                    caller,(char*) (caller->tc_Node.ln_Name ? caller->tc_Node.ln_Name : ""),screen);

            ReturnBool("closescreen",FALSE);
        }
    }
#endif

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockPubScreenList\n"));
    LockPubScreenList();
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockPubScreenList done\n"));

    msg.Screen = screen;
    DoSyncAction((APTR)int_closescreen,&msg.msg,IntuitionBase);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockPubScreenList\n"));
    UnlockPubScreenList();
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockPubScreenList done\n"));

    if (IS(screen)->pubScrNode != NULL)
    {
#ifdef USEWINDOWLOCK
        if (IS(screen)->WindowLock) UNLOCKWINDOW;
#endif
        DEBUG_CLOSESCREEN(dprintf("CloseScreen: failed\n"));
        //something went wrong! int_closescreen is supposed to clear pubScrNode ptr!
        return FALSE;
    }

    /* AROS: Notify decorator */
    int_ExitDecorator(screen);

    /* kill screen bar */
    KillScreenBar(screen, IntuitionBase);

    /* kill depth gadget */
    if (((struct IntScreen *)screen)->depthgadget)
    {
        Object *im = (Object*)((struct Gadget *)(((struct IntScreen *)screen)->depthgadget))->GadgetRender;

        DisposeObject(im);
        DisposeObject(((struct IntScreen *)screen)->depthgadget);
    }

    if (IS(screen)->RestoreDBufInfo != NULL)
    {
        FreeDBufInfo(IS(screen)->RestoreDBufInfo);
    }

    RethinkDisplay();
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: Rethink done\n"));
    
    FreeVPortCopLists(&screen->ViewPort);

#ifdef __MORPHOS__
    /*
     * AROS: According to documentation, VTAG_ATTACH_CM_GET should return a pointer
     * to our ViewPort. ViewPort is part of struct Screen, so there's nothing to
     * GfxFree(). Also, ViewPortExtra is GfxFree()d in FreeColorMap(), attempt to
     * free it twice doesn't do good things. Looks like some MorphOS quirk. Note
     * that the same thing is surrounded by #ifdef __MORPHOS__ in original MorphOS code.
     * TODO: Check if our behavior is correct and adjust it to correspond AmigaOS v3.
     */
    {
        struct TagItem tags[3];

        tags[0].ti_Tag = VTAG_ATTACH_CM_GET;
        tags[0].ti_Data = 0;
        tags[1].ti_Tag = VTAG_VIEWPORTEXTRA_GET;
        tags[1].ti_Data = 0;
        tags[2].ti_Tag = VTAG_END_CM;

        if (VideoControl(screen->ViewPort.ColorMap, tags) == NULL)
        {
            DEBUG_CLOSESCREEN(dprintf("CloseScreen: CM %lx EX %lx\n",tags[0].ti_Data,tags[1].ti_Data));
            if (tags[0].ti_Data) GfxFree((APTR)tags[0].ti_Data);
            if (tags[1].ti_Data) GfxFree((APTR)tags[1].ti_Data);
        }
    }
#endif

#ifdef USEWINDOWLOCK
    if (IS(screen)->WindowLock) UNLOCKWINDOW;
#endif

    /* Free the RasInfo of the viewport */
    FreeMem(screen->ViewPort.RasInfo, sizeof (struct RasInfo));

#if 0
    /* Root layer now automatically freed in ThinLayerInfo() */

#ifdef CreateLayerTagList
    /* Free the root layer */
    DeleteLayer(0UL, ((struct IntScreen *)screen)->rootLayer);
#endif
#endif
    /* Uninit the layerinfo */

    ThinLayerInfo(&screen->LayerInfo);
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: ThinLayer done\n"));

    ReadPixel(&screen->RastPort,0,0);

    /* Free the screen's bitmap */
    if (IS(screen)->AllocatedBitMap)
    {
        FreeBitMap(IS(screen)->AllocatedBitMap);
    }

    screen->RastPort.BitMap = NULL;

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: Freebitmap done\n"));

    /* Free the RastPort's contents */
    DeinitRastPort(&screen->RastPort);

    if (((struct IntScreen *)screen)->DInfo.dri_Customize)
    {
        /* Free the skin */
                /* AROS: submenu image moved out of #ifdef */
        DisposeObject(((struct IntScreen *)screen)->DInfo.dri_Customize->submenu);
#ifdef SKINS
        DisposeObject(((struct IntScreen *)screen)->DInfo.dri_Customize->menutoggle);
        int_SkinAction(SKA_FreeSkin,(ULONG*)&((struct IntScreen *)(screen))->DInfo,(struct Screen *)screen,IntuitionBase);
        int_FreeTitlebarBuffer(screen,IntuitionBase);
#endif
        FreeMem(((struct IntScreen *)screen)->DInfo.dri_Customize,sizeof (struct IntuitionCustomize));
    }

#ifdef SKINS
    if (((struct IntScreen *)screen)->DInfo.dri_Colors)
    {
        FreeMem(((struct IntScreen *)screen)->DInfo.dri_Colors,4 * DRIPEN_NUMDRIPENS);
    }
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: skins done\n"));
#endif

    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_CheckMark);
    DisposeObject(((struct IntScreen *)screen)->DInfo.dri_AmigaKey);

    /* Close the font */
    CloseFont(((struct IntScreen *)screen)->DInfo.dri_Font);

    /* Free the ColorMap */
    FreeColorMap(screen->ViewPort.ColorMap);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: Freecolormap done\n"));

    /* Free the sprite */
    ReleaseSharedPointer(((struct IntScreen *)screen)->Pointer, IntuitionBase);

    /* Free the memory */
    DisposeObject(screen);

    /* AROS: Send notification */
    if (screen != GetPrivIBase(IntuitionBase)->WorkBench)
        FireScreenNotifyMessage((IPTR) screen, SNOTIFY_AFTER_CLOSESCREEN, IntuitionBase);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: ok\n"));

    ReturnBool("CloseScreen",TRUE);
 
    AROS_LIBFUNC_EXIT
} /* CloseScreen */

static VOID int_closescreen(struct CloseScreenActionMsg *msg,
                            struct IntuitionBase *IntuitionBase)
{
    struct Screen *parent,*screen = msg->Screen;
    ULONG lock = 0;

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: welcome in inputhandler!\n"));

#ifdef SKINS
    if (IntuitionBase->FirstScreen == screen) ScreenDepth(screen,SDEPTH_TOBACK,NULL);
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: put the screen to back\n"));
#endif

#if USE_NEWDISPLAYBEEP
    /* we could be beeping this screen... */
    /* NOTE: we're running under the windowlock here! */
    if (IBase->BeepingScreens && (screen->Flags & BEEPING))
    {
        IBase->BeepingScreens --;
    }
#endif

    if (IBase->MenuVerifyScreen == IS(screen))
            IBase->MenuVerifyScreen = NULL;

    /* If this is a public screen, free related information if there are
       no windows left on the screen */
    if (IS(screen)->pubScrNode != NULL)
    {
        DEBUG_CLOSESCREEN(dprintf("CloseScreen: killing a pubscreen entry\n"));

        if(IS(screen)->pubScrNode->psn_VisitorCount || screen->FirstWindow)
        {
            DEBUG_CLOSESCREEN(dprintf("CloseScreen: failed\n"));
            return;
        }

        Remove((struct Node *)IS(screen)->pubScrNode);

        FreeVec(IS(screen)->pubScrNode->psn_Node.ln_Name);

        FreeMem(IS(screen)->pubScrNode,
                sizeof(struct PubScreenNode));

        IS(screen)->pubScrNode = 0;
    }

    RemoveResourceFromList(screen, RESOURCE_SCREEN, IntuitionBase);

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: LockIBase\n"));

    if (!ILOCKCHECK(((struct IntuiActionMsg *)msg))) lock = LockIBase(0);

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
            if (IBase->DefaultPubScreen == screen)
            {
                struct Screen *scr;

                IBase->DefaultPubScreen = NULL;
                scr = IntuitionBase->FirstScreen;

                if (scr && IBase->IControlPrefs.ic_Flags & ICF_DEFPUBSCREEN && IS(scr)->pubScrNode && (scr->Flags & (PUBLICSCREEN | WBENCHSCREEN)))
                {
                    IBase->DefaultPubScreen = scr;
                }
            }

            DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase\n"));
            if (!ILOCKCHECK(((struct IntuiActionMsg *)msg))) UnlockIBase(lock);
            DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase done\n"));

#ifdef TIMEVALWINDOWACTIVATION
            if (IntuitionBase->FirstScreen && ((IBase->IControlPrefs.ic_Flags & ICF_SCREENACTIVATION) || !IntuitionBase->ActiveWindow))
            {
                DeactivateWindow(NULL,msg->msg.task,IntuitionBase);
            }
#endif
            return;
        }
        parent = parent->NextScreen;
    }

    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase\n"));
    if (!ILOCKCHECK(((struct IntuiActionMsg *)msg))) UnlockIBase(lock);
    DEBUG_CLOSESCREEN(dprintf("CloseScreen: UnLockIBase done\n"));

    return;
}
