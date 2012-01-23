/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#include <proto/input.h>
#include <proto/timer.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/pointerclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include <clib/macros.h>
#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "inputhandler_support.h"
#include "inputhandler_actions.h"
#include "menus.h"
#include "monitorclass_private.h"

#ifdef SKINS
#   include "smallmenu.h"
#   include "intuition_customizesupport.h"
#endif

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define DEBUG_HANDLER(x)    ;
#define DEBUG_KEY(x)        ;
#define DEBUG_SCREENKEY(x)  ;
#define DEBUG_AUTOSCROLL(x)
#define DEBUG_CLICK(x)
#define DEBUG_DRAG(x)
#define DEBUG_GADGET(x)
#define DEBUG_MONITOR(x)
#define DEBUG_MOUSE(x)
#define DEBUG_WINDOW(x)

/****************************************************************************************/

struct Interrupt *InitIIH(struct IntuitionBase *IntuitionBase)
{
    struct Interrupt *iihandler;

    D(bug("InitIIH(IntuitionBase=%p)\n", IntuitionBase));

    iihandler = AllocMem(sizeof (struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    if (iihandler)
    {
        struct IIHData   *iihdata;

        iihdata = AllocMem(sizeof (struct IIHData), MEMF_PUBLIC | MEMF_CLEAR);
        if (iihdata)
        {
            struct MsgPort *port;

            port = AllocMem(sizeof (struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
            if (port)
            {
                if ((iihdata->InputEventMemPool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
                                  sizeof(struct GeneratedInputEvent) * 10,
                                  sizeof(struct GeneratedInputEvent) * 10)) &&
                    (iihdata->ActionsMemPool = CreatePool(MEMF_SEM_PROTECTED,
                                  2000, 2000)))
                {
                    const struct TagItem dragtags[] =
                    {
                        {GA_SysGadget   , TRUE          },
                        {GA_SysGType    , GTYP_WDRAGGING},
                        {TAG_DONE                       }
                    };

                    const struct TagItem sizetags[] =
                    {
                        {GA_SysGadget   , TRUE          },
                        {GA_SysGType    , GTYP_SIZING   },
                        {TAG_DONE                       }
                    };

                    iihdata->MasterDragGadget = (struct Gadget *)NewObjectA(GetPrivIBase(IntuitionBase)->dragbarclass,
                                    NULL,
                                    (struct TagItem *)dragtags);

                    iihdata->MasterSizeGadget = (struct Gadget *)NewObjectA(GetPrivIBase(IntuitionBase)->sizebuttonclass,
                                    NULL,
                                    (struct TagItem *)sizetags);

                    if (iihdata->MasterDragGadget && iihdata->MasterSizeGadget)
                    {
                        ULONG lock;

                        /* We do not want to be woken up by message replies.
                           We are anyway woken up about 10 times a second by
                           timer events
                           */
                        port->mp_Flags   = PA_IGNORE;

                        NEWLIST( &(port->mp_MsgList) );
                        iihdata->IntuiReplyPort = port;

                        NEWLIST((struct List*) &iihdata->IntuiActionQueue);
                        NEWLIST((struct List*)&iihdata->NewAllocatedInputEventList);
                        NEWLIST((struct List*)&iihdata->AllocatedInputEventList);
                        iihdata->EndInputEventChain = &iihdata->ReturnInputEvent;
                        iihdata->FreeInputEvents = NULL;

                        iihdata->ActQualifier = IEQUALIFIER_RELATIVEMOUSE;

                        /* Note: there are several routines like CloseWindow, which
                           expect is_Data to point to the IIHData structure, so don't
                           change this! */

                        iihandler->is_Code = (APTR)AROS_ASMSYMNAME(IntuiInputHandler);
                        iihandler->is_Data = iihdata;
                        iihandler->is_Node.ln_Pri   = 50;
                        iihandler->is_Node.ln_Name  = "Intuition InputHandler";

                        lock = LockIBase(0UL);

                        iihdata->IntuitionBase = IntuitionBase;

                        UnlockIBase(lock);

                        GetPrivIBase(IntuitionBase)->IntuiReplyPort = iihdata->IntuiReplyPort;
                        GetPrivIBase(IntuitionBase)->IntuiActionQueue = &iihdata->IntuiActionQueue;

                        ReturnPtr ("InitIIH", struct Interrupt *, iihandler);
                    } /* f (iihdata->MasterDragGadget && iihdata->MasterSizeGadget) */

                    DisposeObject((Object *)iihdata->MasterDragGadget);
                    DisposeObject((Object *)iihdata->MasterSizeGadget);

                    DeletePool(iihdata->ActionsMemPool);
                    DeletePool(iihdata->InputEventMemPool);

                } /* if (iihdata->InputEventMemPool = ... */
                FreeMem(port, sizeof(struct MsgPort));

            } /* if (port) */
            FreeMem(iihdata, sizeof (struct IIHData));
            iihdata->MouseBoundsActiveFlag = FALSE;

    } /* if (iihdata) */
        FreeMem(iihandler, sizeof (struct Interrupt));

    } /* if (iihandler) */

    ReturnPtr ("InitIIH", struct Interrupt *, NULL);
}

/****************************************************************************************/

VOID CleanupIIH(struct Interrupt *iihandler, struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihdata = (struct IIHData *)iihandler->is_Data;

    DisposeObject((Object *)iihdata->MasterDragGadget);
    DisposeObject((Object *)iihdata->MasterSizeGadget);

    FreeGeneratedInputEvents(iihdata);
    DeletePool(iihdata->InputEventMemPool);
    DeletePool(iihdata->ActionsMemPool);

    /* One might think that this port is still in use by the inputhandler.
    ** However, if intuition is closed for the last time, there should be no
    ** windows that IntuiMessage can be sent to.
    */
    FreeMem(iihdata->IntuiReplyPort, sizeof (struct MsgPort));

    FreeMem(iihdata, sizeof (struct IIHData));
    FreeMem(iihandler, sizeof (struct Interrupt));

    return;
}

/****************************************************************************************/

static void HandleIntuiReplyPort(struct IIHData *iihdata, struct IntuitionBase *IntuitionBase)
{
    struct Library *TimerBase = GetPrivIBase(IntuitionBase)->TimerBase;
    struct IntuiMessage *im;

    while ((im = (struct IntuiMessage *)GetMsg(iihdata->IntuiReplyPort)))
    {
        if (im->IDCMPWindow && ResourceExisting(im->IDCMPWindow, RESOURCE_WINDOW, IntuitionBase))
        {
            struct IntWindow *win = (struct IntWindow *)im->IDCMPWindow;

            Forbid();
            GetSysTime(&win->lastmsgreplied);
            Permit();

            switch(im->Class)
            {
            case IDCMP_MOUSEMOVE:
                    IW(im->IDCMPWindow)->num_mouseevents--;
                    break;

            case IDCMP_INTUITICKS:
                    AROS_ATOMIC_AND(im->IDCMPWindow->Flags, ~WFLG_WINDOWTICKED);
                    break;

            #if USE_IDCMPUPDATE_MESSAGECACHE
            case IDCMP_IDCMPUPDATE:
                    IW(im->IDCMPWindow)->num_idcmpupdate--;

                    if (!(IW(im->IDCMPWindow)->num_idcmpupdate) && IW(im->IDCMPWindow)->messagecache)
                    {
                    SendIntuiMessage(im->IDCMPWindow,IW(im->IDCMPWindow)->messagecache);
                    IW(im->IDCMPWindow)->messagecache = 0;
                    }
                    break;
            #endif
        
            case IDCMP_MENUVERIFY:
                {
                    struct Window    *w = im->IDCMPWindow;
                    struct IntScreen *scr = 0;

                    scr = GetPrivScreen(w->WScreen);

                    if (scr != GetPrivIBase(IntuitionBase)->MenuVerifyScreen ||
                        scr->MenuVerifySeconds > im->Seconds ||
                        (scr->MenuVerifySeconds == im->Seconds &&
                         scr->MenuVerifyMicros > im->Micros))
                    {
                        /* The timeout has expired, just ignore. */
                    }
                    else
                    {
                        --scr->MenuVerifyMsgCount;
                        if (w == scr->MenuVerifyActiveWindow &&
                            im->Code == MENUCANCEL)
                        {
                            ULONG          lock = LockIBase(0);
                            struct Window *w1;

                            for (w1 = scr->Screen.FirstWindow; w1; w1 = w1->NextWindow)
                            {
                                if (w1->IDCMPFlags & IDCMP_MENUVERIFY && w1 != scr->MenuVerifyActiveWindow)
                                {
                                    ih_fire_intuimessage(w1,
                                                 IDCMP_MOUSEBUTTONS,
                                                 MENUUP,
                                                 w1,
                                                 IntuitionBase);
                                }
                            }

                            UnlockIBase(lock);

                            scr->MenuVerifyActiveWindow = NULL;
                            scr->MenuVerifyTimeOut = 0;
                            scr->MenuVerifyMsgCount = 0;
                            scr->MenuVerifySeconds = 0;
                            scr->MenuVerifyMicros = 0;
                            GetPrivIBase(IntuitionBase)->MenuVerifyScreen = NULL;
                        }
                        else if (scr->MenuVerifyMsgCount == 0)
                        {
                            struct InputEvent ie;

                            /* currently we ONLY need the menu open time ! */
                            ie.ie_TimeStamp.tv_secs = im->Seconds;
                            ie.ie_TimeStamp.tv_micro = im->Micros;

                            if (FireMenuMessage(MMCODE_START, scr->MenuVerifyActiveWindow, &ie, IntuitionBase))
                            {
                                /* This lock will be released only when the user is
                                   done with menus = when IECLASS_MENU + IESUBCLASS_MENUSTOP
                                   event arrives (generated by MenuHandler task) */

                                ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
                                iihdata->MenuWindow = scr->MenuVerifyActiveWindow;
                                MENUS_ACTIVE = TRUE;
                            }

                            scr->MenuVerifyActiveWindow = NULL;
                            scr->MenuVerifyTimeOut = 0;
                            scr->MenuVerifyMsgCount = 0;
                            scr->MenuVerifySeconds = 0;
                            scr->MenuVerifyMicros = 0;
                            GetPrivIBase(IntuitionBase)->MenuVerifyScreen = NULL;
                        }
                    }
                    break;
                }

            case IDCMP_SIZEVERIFY:
                {
                    struct GadgetInfo     *gi = &iihdata->GadgetInfo;
                    struct Window       *w = im->IDCMPWindow;
                    struct Gadget       *gadget = im->IAddress;
                    struct InputEvent      ie;
                    BOOL                 reuse_event;

                    PrepareGadgetInfo(gi, IntuitionBase->ActiveScreen, w, NULL);
                    SetGadgetInfoGadget(gi, gadget, IntuitionBase);
            
                    if (IS_BOOPSI_GADGET(gadget))
                    {
                        ie.ie_NextEvent = NULL;
                        ie.ie_Class     = IECLASS_RAWMOUSE;
                        ie.ie_SubClass  = 0;
                        ie.ie_Code      = IECODE_LBUTTON;
                        ie.ie_Qualifier = im->Qualifier;
                        ie.ie_X         = im->MouseX;
                        ie.ie_Y         = im->MouseY;
                        ie.ie_TimeStamp.tv_secs  = IntuitionBase->Seconds;
                        ie.ie_TimeStamp.tv_micro = IntuitionBase->Micros;
            
                        DoGPInput(gi,
                              gadget,
                              &ie,
                              GM_GOACTIVE,
                              &reuse_event,
                              IntuitionBase);
                  
                        /* For compatibility, send a GM_HANDLEINPUT too */
                        ie.ie_Class = IECLASS_RAWMOUSE;
                        ie.ie_Code  = IECODE_NOBUTTON;
                        ie.ie_X     = 0;
                        ie.ie_Y     = 0;
            
                        gadget = DoGPInput(gi,
                                   gadget,
                                   &ie,
                                   GM_HANDLEINPUT,
                                   &reuse_event,
                                   IntuitionBase);
                    }

                    /* From now on the master drag/size gadget takes over */

                    iihdata->ActiveSysGadget = gadget;
                    gadget = iihdata->MasterSizeGadget;
                    iihdata->ActiveGadget = gadget;

                    ie.ie_Class = IECLASS_RAWMOUSE;
                    ie.ie_Code  = IECODE_LBUTTON;
                    ie.ie_X     = im->MouseX;
                    ie.ie_Y     = im->MouseY;
            
                    DoGPInput(gi,
                          gadget,
                          &ie,
                          GM_GOACTIVE,
                          &reuse_event,
                          IntuitionBase);
                    break;
                }

            case IDCMP_REQVERIFY:
                {
                    struct Window *w = im->IDCMPWindow;

                    EndRequest(w->DMRequest, w);

                    ih_fire_intuimessage(w,
                                     IDCMP_REQSET,
                                     0,
                                     w,
                                     IntuitionBase);
                }
                break;

            case IDCMP_WBENCHMESSAGE:
                    DEBUG_WORKBENCH(dprintf("HandleIntuiReplyPort: code 0x%lx\n",
                            im->Code));
                    break;

            } /* switch(im->Class) */

            if (im->Qualifier & IEQUALIFIER_REPEAT)
            {
                /* IDCMP_IDCMPUPDATE messages can also be sent from app task, therefore
                   it would be better if there was an ATOMIC_DEC macro or something */

                if (IW(im->IDCMPWindow)->num_repeatevents)
                {
                    IW(im->IDCMPWindow)->num_repeatevents--;
                }
            }
        }
        FreeIntuiMessage(im);

    } /* while ((im = (struct IntuiMessage *)GetMsg(iihdata->IntuiReplyPort))) */
}
/****************************************************************************************/

struct Window *GetToolBoxWindow(struct InputEvent *ie, struct Screen *scr, struct IntuitionBase *IntuitionBase)
{
    /* The caller has checked that the input event is a IECLASS_RAWMOUSE, SELECTDOWN event */
    /* NOTE: may be called with NULL ie ptr! */
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Layer    *l;
    struct Window   *new_w = NULL;

    if (scr)
    {
        D(bug("GetToolBoxWindow: Click at (%d,%d)\n",scr->MouseX,scr->MouseY));

        /* What layer ? */
        LockLayerInfo(&scr->LayerInfo);

        l = WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);

        UnlockLayerInfo(&scr->LayerInfo);

        if (NULL == l)
        {
            new_w = NULL;
            D(bug("GetToolBoxWindow: Click not inside layer\n"));
        }
        else if (l == scr->BarLayer)
        {
        }
        else
        {
            new_w = (struct Window *)l->Window;
            if (new_w)
            {
                if ((new_w->Flags & WFLG_TOOLBOX) == 0) new_w = NULL;
            }
            if (!new_w)
            {
                D(bug("GetToolBoxWindow: Selected layer is not a ToolBox window\n"));
            }
            else
            {
                D(bug("GetToolBoxWindow: Found layer %p\n", l));
            }
        }
    }

    D(bug("GetToolBoxWindow: New window %p\n", new_w));
    return new_w;
}

/****************************************************************************************/

#define KEY_QUALIFIERS (IEQUALIFIER_LSHIFT     | IEQUALIFIER_RSHIFT   | \
IEQUALIFIER_CAPSLOCK   | IEQUALIFIER_CONTROL  | \
IEQUALIFIER_LALT       | IEQUALIFIER_RALT     | \
IEQUALIFIER_LCOMMAND   | IEQUALIFIER_RCOMMAND | \
IEQUALIFIER_NUMERICPAD)

#define BUTTON_QUALIFIERS (IEQUALIFIER_MIDBUTTON | IEQUALIFIER_RBUTTON | IEQUALIFIER_LEFTBUTTON)

static struct Gadget *Process_RawMouse(struct InputEvent *ie, struct IIHData *iihdata, struct Screen *screen,
                       struct Window *w, struct Gadget *gadget, struct GadgetInfo *gi,
                       ULONG stitlebarhit, BOOL new_active_window, BOOL IsToolbox,
                       struct InputEvent *orig_ie, BOOL *keep_event, BOOL *reuse_event,
#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
                       BOOL *call_setpointerpos,
#endif
                       struct IntuitionBase *IntuitionBase)
{
    struct Library *InputBase = GetPrivIBase(IntuitionBase)->InputBase;
    struct Requester *req = w ? w->FirstRequest : NULL;

    switch (ie->ie_Code) {
    case SELECTDOWN:
    {
        BOOL new_gadget = FALSE;
        BOOL sizeverify = FALSE;
        UWORD MetaDrag = GetPrivIBase(IntuitionBase)->IControlPrefs.ic_MetaDrag;

        DEBUG_CLICK(bug("[Inputhandler] Screen 0x%p, Window 0x%p, Gadget 0x%p, screen titlebar %d, new active window %d\n", screen, w, gadget, stitlebarhit, new_active_window));
        DEBUG_CLICK(if (screen) bug("[Inputhandler] Coordinates: (%d, %d)\n", screen->MouseX, screen->MouseY));
            
        iihdata->ActQualifier |= IEQUALIFIER_LEFTBUTTON;

        /* Enter screen dragging mode if LButton + MetaDrag are pressed. */
        if (MetaDrag && ((iihdata->ActQualifier & KEY_QUALIFIERS) == MetaDrag)) {
            iihdata->ScreenDrag = screen;
            *keep_event = FALSE;
        break;
        }

#ifdef SKINS
        iihdata->TitlebarAppearTime = 0;
#endif
        if (MENUS_ACTIVE)
        {
        FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
        *keep_event = FALSE;
        break;
        }

#ifdef SKINS
        if (!gadget && w)
        {
        struct Gadget * draggadget = 0;

        if ((!(w->FirstRequest)) && (w->Flags & WFLG_DRAGBAR) && MatchHotkey(ie,IA_ACTIVEWINDOWMOVE,IntuitionBase))
        {
            if (w->MouseX < IW(w)->sizeimage_width || w->MouseX > w->Width - IW(w)->sizeimage_width - 1 || w->MouseY < ((IW(w)->sizeimage_height > w->BorderTop) ? IW(w)->sizeimage_height : w->BorderTop) || w->MouseY > w->Height - IW(w)->sizeimage_height - 1)
            {
            for (draggadget = w->FirstGadget; draggadget; draggadget = draggadget->NextGadget)
            {
                if ((draggadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING)
                {
                gadget = draggadget;
                new_gadget = TRUE;
                break;
                }
            }
            }
        }

        if ((!(w->FirstRequest)) && (w->Flags & WFLG_SIZEGADGET) && MatchHotkey(ie,IA_ACTIVEWINDOWSIZE,IntuitionBase))
        {
            if (w->MouseX < IW(w)->sizeimage_width || w->MouseX > w->Width - IW(w)->sizeimage_width - 1 || w->MouseY < ((IW(w)->sizeimage_height > w->BorderTop) ? IW(w)->sizeimage_height : w->BorderTop) || w->MouseY > w->Height - IW(w)->sizeimage_height - 1)
            {
            for (draggadget = w->FirstGadget; draggadget; draggadget = draggadget->NextGadget)
            {
                if ((draggadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SIZING)
                {
                gadget = draggadget;
                new_gadget = TRUE;
                break;
                }
            }
            }
        }
        }
#endif

        if (!gadget)
        {
        /* use the *current* screen rather than active one when searching
           for sdepth gadget! */
        if (screen)
            gadget = FindGadget (screen, stitlebarhit ? NULL : w, stitlebarhit ? NULL : req,
                     screen->MouseX, screen->MouseY, gi, FALSE, IntuitionBase);
        DEBUG_CLICK(bug("Click on gadget %p\n", gadget));
        new_gadget = TRUE;

        } /* if (!gadget) */

        /* If we clicked screen titlebar outside of any gadget, enter drag mode */
        if ((!gadget) && stitlebarhit) {
        DEBUG_CLICK(bug("[Inputhandler] Entering drag state for screen 0x%p\n", screen));
            iihdata->ScreenDrag = screen;
            *keep_event = FALSE;
        break;
        }

        if (!IsToolbox) {
            if (!gadget && stitlebarhit)
            {
            struct Window *ww = 0;
        
            ww = FindDesktopWindow(screen, IntuitionBase);
            DEBUG_CLICK(bug("[Inputhandler] Clicked on backdrop window 0x%p\n", ww));
            if (ww) {
                ActivateWindow(ww);
                w = ww;
            }
            }

            if (!stitlebarhit && !new_active_window && DoubleClick(GetPrivIBase(IntuitionBase)->LastClickSecs,GetPrivIBase(IntuitionBase)->LastClickMicro,
                                        ie->ie_TimeStamp.tv_secs,ie->ie_TimeStamp.tv_micro))
            {
            if (GetPrivIBase(IntuitionBase)->DoubleClickButton != SELECTDOWN)
            {
                GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
                GetPrivIBase(IntuitionBase)->DoubleClickButton = SELECTDOWN;
            }
            else
            {
                GetPrivIBase(IntuitionBase)->DoubleClickCounter ++;
            }
            }
            else
            {
            DEBUG_CLICK(bug("[Inputhandler] Resetting doubleclick counter\n"));
            GetPrivIBase(IntuitionBase)->DoubleClickButton = SELECTDOWN;
            GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
            }

            /* update last click time for doubleclicktofront */
            GetPrivIBase(IntuitionBase)->LastClickSecs = ie->ie_TimeStamp.tv_secs;
            GetPrivIBase(IntuitionBase)->LastClickMicro = ie->ie_TimeStamp.tv_micro;

#ifdef SKINS
            if (!stitlebarhit)
            {
            ULONG result;
        
            if (!(gadget && ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDEPTH)))
            if ((result = RunHotkeys(ie,IntuitionBase)))
            {
                //gadget = NULL;
                if (result == RUNHOTREUSE)
                {
                *reuse_event = TRUE;
                }
                else
                {
                *keep_event = FALSE;
                }
            }
            }
#endif
        }
        if (gadget && new_gadget)
        {
        DEBUG_GADGET(bug("[Inputhandler] Activate gadget: 0x%p\n", gadget));
        if (w && (w->IDCMPFlags & IDCMP_SIZEVERIFY) &&
            (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SIZING /*||
                     (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WZOOM*/)
        {
            ih_fire_intuimessage(w,
                     IDCMP_SIZEVERIFY,
                     0,
                     gadget,
                     IntuitionBase);
            gadget = NULL;
            sizeverify = TRUE;
        }
        else
        {
            BOOL is_draggad, is_sizegad;
            UWORD gsystype;

            /* Whenever the active gadget changes the gi must be updated
               because it is cached in iidata->GadgetInfo!!!! Don't
               forget to do this if somewhere else the active
               gadget is changed, for example in ActivateGadget!!! */

            PrepareGadgetInfo(gi, IntuitionBase->ActiveScreen, w, req);
            SetGadgetInfoGadget(gi, gadget, IntuitionBase);
            
            gsystype = gadget->GadgetType & GTYP_SYSTYPEMASK;
            is_draggad = ((gsystype == GTYP_WDRAGGING)
#ifdef __MORPHOS__
                  || (gsystype == GTYP_WDRAGGING2)
#endif
                 );
            is_sizegad = (gsystype == GTYP_SIZING);

            /* jDc: intui68k sends IDCMPs for GACT_IMMEDIATE drag&sizegads! */
            if (gadget->Activation & GACT_IMMEDIATE)
            {
            ih_fire_intuimessage(w,
                         IDCMP_GADGETDOWN,
                         0,
                         gadget,
                         IntuitionBase);
            }

            if (is_draggad || is_sizegad)
            {
            if (IS_BOOPSI_GADGET(gadget))
            {
                DoGPInput(gi,
                      gadget,
                      ie,
                      GM_GOACTIVE,
                      reuse_event,
                      IntuitionBase);

                /* Ignoring retval of dispatcher above is what
                   AmigaOS does too for boopsi drag/resize
                   gadgets */

            }

            /* From now on the master drag/size gadget takes over */
            if ((w->MoreFlags & WMFLG_IAMMUI) && (w->Flags & WFLG_BORDERLESS))
                iihdata->ActiveSysGadget = is_draggad ? gadget : 0;
            else
                iihdata->ActiveSysGadget = gadget;
            gadget = is_draggad ? iihdata->MasterDragGadget : iihdata->MasterSizeGadget;
            }
        }
        }

        if (gadget)
        {

        switch (gadget->GadgetType & GTYP_GTYPEMASK)
        {
        case GTYP_BOOLGADGET:
            /* Only set the GFLG_SELECTED flag for RELVERIFY and
             * TOGGLESELECT gadget. It's for the user to do it if
             * he wants for other GADGIMMEDIATE ones.
             * Only RELVERIFY gadgets stay active.
             */

            if (gadget->Activation & (GACT_TOGGLESELECT | GACT_RELVERIFY))
            {
            gadget->Flags ^= GFLG_SELECTED;
            RefreshBoolGadgetState(gadget, w, req, IntuitionBase);
            }

            if (gadget->Activation & GACT_RELVERIFY)
            {
            gadget->Activation |= GACT_ACTIVEGADGET;
            iihdata->MouseWasInsideBoolGadget = TRUE;
            }
            else
            {
            gadget = NULL;
            }
            break;

        case GTYP_PROPGADGET:
            HandlePropSelectDown(gadget,
                 w,
                 req,
                 w->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, gi->gi_Screen, gi->gi_Window, NULL),
                 w->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, gi->gi_Screen, gi->gi_Window, NULL),
                 IntuitionBase);


            break;

        case GTYP_STRGADGET:
            /* If the click was inside the active strgad,
            ** then let it update cursor pos,
            ** else deactivate stringadget and reuse event.
            */

            if (InsideGadget(gi->gi_Screen, gi->gi_Window,
                     gi->gi_Requester, gadget,
                     gi->gi_Screen->MouseX, gi->gi_Screen->MouseY))
            {
            UWORD imsgcode;

            HandleStrInput(gadget, gi, ie, &imsgcode,
                   IntuitionBase);
            }
            else
            {
            gadget->Flags &= ~GFLG_SELECTED;

            RefreshStrGadget(gadget, w, req, IntuitionBase);
            /* Gadget not active anymore */
            gadget = NULL;
            *reuse_event = TRUE;
            }
            break;

        case GTYP_CUSTOMGADGET:
            gadget = DoGPInput(gi,
                   gadget,
                   ie,
                   (new_gadget ? GM_GOACTIVE : GM_HANDLEINPUT),
                   reuse_event,
                   IntuitionBase);
            D(bug("new_gadget %d, goactive %p\n", new_gadget, gadget));

            if (gadget && new_gadget && (!(gadget->GadgetType & GTYP_SIZING)))
            {
            /* For compatibility, send a GM_HANDLEINPUT too */
            struct InputEvent newie;
            BOOL               reuse_event;
            
            newie.ie_NextEvent  = NULL;
            newie.ie_Class      = IECLASS_RAWMOUSE;
            newie.ie_SubClass   = 0;
            newie.ie_Code         = IECODE_NOBUTTON;
            newie.ie_Qualifier  = ie->ie_Qualifier;
            newie.ie_X          = 0;
            newie.ie_Y          = 0;
            newie.ie_TimeStamp.tv_secs  = IntuitionBase->Seconds;
            newie.ie_TimeStamp.tv_micro = IntuitionBase->Micros;
            
            gadget = DoGPInput(gi,
                   gadget,
                   &newie,
                   GM_HANDLEINPUT,
                   &reuse_event,
                   IntuitionBase);
            D(bug("handleinput %p\n", gadget));
            }
            break;

        case    0: //orig gadtools / some 1.3 gadgets
            if (IS_SYS_GADGET(gadget))
            {
            HandleSysGadgetVerify(gi, gadget, IntuitionBase);
            }
            else
            {
            if (gadget->Activation & GACT_RELVERIFY)
            {
                gadget->Activation |= GACT_ACTIVEGADGET;
                iihdata->MouseWasInsideBoolGadget = TRUE;
                if (gadget->Flags & GFLG_GADGHIMAGE)
                {
                gadget->Flags ^= GFLG_SELECTED;
                RefreshBoolGadgetState(gadget, w, req, IntuitionBase);
                }
            }
            else
            {
                /* jDc: this is what original intuition does, before crashing after a while ;)*/
                DEBUG_CLICK(bug("[Inputhandler] Sending SELECTDOWN (old gadget), window 0x%p\n", w));
                ih_fire_intuimessage(w,
                         IDCMP_MOUSEBUTTONS,
                         SELECTDOWN,
                         w,
                         IntuitionBase);
            }
            }
            break;
        } /* switch (GadgetType) */

        } /* if (a gadget is active) */
        else if (w && (!req || req->Flags & NOISYREQ) && !sizeverify && !stitlebarhit)
        {
        DEBUG_CLICK(bug("[Inputhandler] Sending SELECTDOWN, window 0x%p\n", w));
        ih_fire_intuimessage(w,
                     IDCMP_MOUSEBUTTONS,
                     SELECTDOWN,
                     w,
                     IntuitionBase);
        }

    } /* case SELECTDOWN */
    break;

    case SELECTUP:
    iihdata->ActQualifier &= ~IEQUALIFIER_LEFTBUTTON;

    /* Ignore this event if screen drag qualifier is pressed */
    if (iihdata->ScreenDrag) {
        iihdata->ScreenDrag = NULL;
        *keep_event = FALSE;
        break;
    }

#ifdef SKINS
    iihdata->TitlebarAppearTime = 0;
#endif
    if (MENUS_ACTIVE)
    {
        FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
        *keep_event = FALSE;
        break;
    }

    if (gadget)
    {
        BOOL inside = InsideGadget(gi->gi_Screen, gi->gi_Window,
                       gi->gi_Requester, gadget,
                       gi->gi_Screen->MouseX, gi->gi_Screen->MouseY);

        /*int selected = (gadget->Flags & GFLG_SELECTED) != 0;*/

        switch (gadget->GadgetType & GTYP_GTYPEMASK)
        {
        case GTYP_BOOLGADGET:
        /* Must be a RELVERIFY gadget */

        if (!(gadget->Activation & GACT_TOGGLESELECT) && inside)
        {
            gadget->Flags ^= GFLG_SELECTED;
            RefreshBoolGadgetState(gadget, w, req, IntuitionBase);
        }

        if (inside)
        {
            if (IS_SYS_GADGET(gadget))
            {
            HandleSysGadgetVerify(gi, gadget, IntuitionBase);
            }
            else
            {
            if (req && gadget->Activation & GACT_ENDGADGET)
            {
                EndRequest(req, w);

                req = w->FirstRequest;
            }

            ih_fire_intuimessage(w,
                        IDCMP_GADGETUP,
                        0,
                        gadget,
                        IntuitionBase);
            }
        }
        else
        {
            /* RKRM say so */
            ih_fire_intuimessage(w,
                     IDCMP_MOUSEBUTTONS,
                     SELECTUP,
                     w,
                     IntuitionBase);
        }

        gadget->Activation &= ~GACT_ACTIVEGADGET;
        gadget = NULL;
        break;

        case GTYP_PROPGADGET:
        HandlePropSelectUp(gadget, w, req, IntuitionBase);
        if (gadget->Activation & GACT_RELVERIFY)
        {
            ih_fire_intuimessage(w,
                     IDCMP_GADGETUP,
                     0,
                     gadget,
                     IntuitionBase);
        }

        gadget = NULL;
        break;

        /* Intuition string gadgets don't care about SELECTUP */

        case GTYP_CUSTOMGADGET:
        gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, reuse_event, IntuitionBase);
        break;

        case 0: //orig gadtools / some 1.3 gadgets
        /* jDc: adding a gadget with gtyp field set to NULL crashes intui68k
        ** seems we don't need compatibility on this field ;) anyway we should
        ** handle the case of GTYP_CLOSE, etc, set by some "cod3r"
        */
        gadget->Activation &= ~GACT_ACTIVEGADGET;
        if (gadget->Activation & GACT_RELVERIFY)
        {
            if (gadget->Flags & GFLG_GADGHIMAGE)
            {
            if (inside)
            {
                gadget->Flags ^= GFLG_SELECTED;
                RefreshBoolGadgetState(gadget, w, req, IntuitionBase);
            }
            }

            if (inside)
            {
            ih_fire_intuimessage(w,
                         IDCMP_GADGETUP,
                         0,
                         gadget,
                         IntuitionBase);
            }
        } else {
            ih_fire_intuimessage(w,
                     IDCMP_MOUSEBUTTONS,
                     SELECTUP,
                     w,
                     IntuitionBase);
        }
        gadget = NULL;
        break;

        } /* switch GadgetType */

    } /* if (a gadget is currently active) */
    else if (w && (!req || req->Flags & NOISYREQ))
    {
        ih_fire_intuimessage(w,
                 IDCMP_MOUSEBUTTONS,
                 SELECTUP,
                 w,
                 IntuitionBase);
    }

    break; /* case SELECTUP */

    case MENUDOWN:
    iihdata->ActQualifier |= IEQUALIFIER_RBUTTON;

#ifdef SKINS
    iihdata->TitlebarAppearTime = 0;
#endif
    if (MENUS_ACTIVE)
    {
        FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
        *keep_event = FALSE;
        break;
    }

    if (DoubleClick(GetPrivIBase(IntuitionBase)->LastClickSecs,GetPrivIBase(IntuitionBase)->LastClickMicro,
            ie->ie_TimeStamp.tv_secs,ie->ie_TimeStamp.tv_micro))
    {
        if (GetPrivIBase(IntuitionBase)->DoubleClickButton != MENUDOWN)
        {
        GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
        GetPrivIBase(IntuitionBase)->DoubleClickButton = MENUDOWN;
        }
        else
        {
        GetPrivIBase(IntuitionBase)->DoubleClickCounter ++;
        }
    }
    else
    {
        GetPrivIBase(IntuitionBase)->DoubleClickButton = MENUDOWN;
        GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
    }

    /* update last click time for doubleclicktofront */
    GetPrivIBase(IntuitionBase)->LastClickSecs = ie->ie_TimeStamp.tv_secs;
    GetPrivIBase(IntuitionBase)->LastClickMicro = ie->ie_TimeStamp.tv_micro;

#ifdef SKINS
    {
        ULONG result;
        
        if ((result = RunHotkeys(ie,IntuitionBase)))
        {
        if (result == RUNHOTREUSE)
        {
            *reuse_event = TRUE;
        }
        else
        {
            *keep_event = FALSE;
        }
        break;
        }
        w = IntuitionBase->ActiveWindow;
    }
#endif

#ifdef SKINS
    if ((!MENUS_ACTIVE) && (!gadget) && (!(iihdata->ActQualifier & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_MIDBUTTON))))
    {
        struct Gadget       *gad;
        struct GadgetInfo      ginf;
        ULONG                hit;
        struct Window       *wind = FindActiveWindow(0, screen, &hit,IntuitionBase);

        if (wind)
        {
        gad = FindGadget (screen,
                  hit ? 0 : wind,0,
                  IntuitionBase->ActiveScreen->MouseX,
                  IntuitionBase->ActiveScreen->MouseY,
                  &ginf, TRUE, IntuitionBase);

        if (gad && ((gad->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDEPTH))
        {
            CreateSmallMenuTask(wind,SMALLMENU_TYPE_WINDOWDEPTH,IntuitionBase);
            *keep_event = FALSE;
            break;
        }
        
        if (gad && ((gad->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SDEPTH))
        {
            CreateSmallMenuTask(0,SMALLMENU_TYPE_SCREENDEPTH,IntuitionBase);
            *keep_event = FALSE;
            break;
        }
        }
    }
#endif

    if (w && !req && w->DMRequest && !(w->Flags & WFLG_RMBTRAP))
    {
        if (!MENUS_ACTIVE &&
        DoubleClick(GetPrivIBase(IntuitionBase)->DMStartSecs,
            GetPrivIBase(IntuitionBase)->DMStartMicro,
            ie->ie_TimeStamp.tv_secs,
            ie->ie_TimeStamp.tv_micro))
        {
        if (w->IDCMPFlags & IDCMP_REQVERIFY)
        {
            ih_fire_intuimessage(w,
                     IDCMP_REQVERIFY,
                     MENUHOT,
                     NULL,
                     IntuitionBase);
        }
        else if (Request(w->DMRequest, w))
        {
            req = w->DMRequest;
            
            ih_fire_intuimessage(w,
                     IDCMP_REQSET,
                     0,
                     w,
                     IntuitionBase);
        }
        *keep_event = FALSE;
        break;
        }

        GetPrivIBase(IntuitionBase)->DMStartSecs  = ie->ie_TimeStamp.tv_secs;
        GetPrivIBase(IntuitionBase)->DMStartMicro = ie->ie_TimeStamp.tv_micro;
    }

    if (w && !gadget)
    {
        if (!(w->Flags & WFLG_RMBTRAP) && !req)
        {
        struct IntScreen *scr = GetPrivScreen(w->WScreen);
        struct Window      *w1;
        ULONG               lock;
        BOOL              mouseon = TRUE;

        scr->MenuVerifyMsgCount = 0;

        if (w->MouseX < 0 || w->MouseY < 0) mouseon = FALSE;
        if (w->MouseX > w->Width || w->MouseY > w->Height) mouseon = FALSE;

        if (w->IDCMPFlags & IDCMP_MENUVERIFY && (!(IW(w)->specialflags & SPFLAG_IAMDEAD)))
        {
            ih_fire_intuimessage(w,
                     IDCMP_MENUVERIFY,
                     mouseon ? MENUHOT : MENUWAITING,
                     w,
                     IntuitionBase);
            scr->MenuVerifyMsgCount++;
        }

        lock = LockIBase(0);

        for (w1 = scr->Screen.FirstWindow; w1; w1 = w1->NextWindow)
        {
            if ((w1->IDCMPFlags & IDCMP_MENUVERIFY) && (w1 != w) && (!(IW(w)->specialflags & SPFLAG_IAMDEAD)))
            {
            ih_fire_intuimessage(w1,
                         IDCMP_MENUVERIFY,
                         MENUWAITING,
                         w1,
                         IntuitionBase);
            ++scr->MenuVerifyMsgCount;
            }
        }

        UnlockIBase(lock);

        /* FIXME: when a window is opened with IDCMP_MENUVERIFY
         * (or this event is requested via ModifyIDCMP), and a
         * verify operation is pending, the window should get
         * a verify message too. Oh well.
         */

        if (scr->MenuVerifyMsgCount)
        {
            GetPrivIBase(IntuitionBase)->MenuVerifyScreen = scr;
            scr->MenuVerifyActiveWindow = w;
            scr->MenuVerifyTimeOut = 2;
            scr->MenuVerifySeconds = IntuitionBase->Seconds;
            scr->MenuVerifyMicros  = IntuitionBase->Micros;
        }
        else if (FireMenuMessage(MMCODE_START, w, NULL/*ie*/, IntuitionBase))
        {
            /* This lock will be released only when the user is
               done with menus = when IECLASS_MENU + IESUBCLASS_MENUSTOP
               event arrives (generated by MenuHandler task) */

            ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
            iihdata->MenuWindow = w;
            MENUS_ACTIVE = TRUE;
        }
        }
    }
    /* fall through */

    case MENUUP:
    case MIDDLEDOWN:
    case MIDDLEUP:

    switch(ie->ie_Code) {
    case MENUUP:
        iihdata->ActQualifier &= ~IEQUALIFIER_RBUTTON;
        if (GetPrivIBase(IntuitionBase)->MenuVerifyScreen)
        {
        struct Window      *w1;
        struct IntScreen *scr = GetPrivIBase(IntuitionBase)->MenuVerifyScreen;
        ULONG               lock = LockIBase(0);

        for (w1 = scr->Screen.FirstWindow; w1; w1 = w1->NextWindow)
        {
            if (w1->IDCMPFlags & IDCMP_MENUVERIFY && w1->IDCMPFlags & IDCMP_MOUSEBUTTONS)
            {
            ih_fire_intuimessage(w1,
                         IDCMP_MOUSEBUTTONS,
                         MENUUP,
                         w1,
                         IntuitionBase);
            }
        }

        UnlockIBase(lock);

        /* FIXME: when the active window replies the verifymessage,
         * it should get a IDCMP_MENUPICK/MENUNULL message.
         */
        GetPrivIBase(IntuitionBase)->MenuVerifyScreen = NULL;
        scr->MenuVerifyActiveWindow = NULL;
        scr->MenuVerifyMsgCount = 0;
        scr->MenuVerifyTimeOut = 0;
        scr->MenuVerifySeconds = 0;
        scr->MenuVerifyMicros = 0;
        }
        break;

    case MIDDLEDOWN:
        iihdata->ActQualifier |= IEQUALIFIER_MIDBUTTON;
        if (DoubleClick(GetPrivIBase(IntuitionBase)->LastClickSecs,GetPrivIBase(IntuitionBase)->LastClickMicro,
            ie->ie_TimeStamp.tv_secs,ie->ie_TimeStamp.tv_micro))
        {
        if (GetPrivIBase(IntuitionBase)->DoubleClickButton != MIDDLEDOWN)
        {
            GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
            GetPrivIBase(IntuitionBase)->DoubleClickButton = MIDDLEDOWN;
        } else
            GetPrivIBase(IntuitionBase)->DoubleClickCounter ++;
        } else {
        GetPrivIBase(IntuitionBase)->DoubleClickButton = MIDDLEDOWN;
        GetPrivIBase(IntuitionBase)->DoubleClickCounter = 0;
        }
        /* update last click time for doubleclicktofront */
        GetPrivIBase(IntuitionBase)->LastClickSecs = ie->ie_TimeStamp.tv_secs;
        GetPrivIBase(IntuitionBase)->LastClickMicro = ie->ie_TimeStamp.tv_micro;
        break;

    case MIDDLEUP:
        iihdata->ActQualifier &= ~IEQUALIFIER_MIDBUTTON;
        break;
    }

#ifdef SKINS
    iihdata->TitlebarAppearTime = 0;
#endif
    if (MENUS_ACTIVE)
    {
        FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
        *keep_event = FALSE;
        break;
    }

#ifdef SKINS
    if (ie->ie_Code == MIDDLEDOWN)
    {
        ULONG result;
        
        if ((result = RunHotkeys(ie,IntuitionBase)))
        {
        if (result == RUNHOTREUSE)
        {
            *reuse_event = TRUE;
        }
        else
        {
            *keep_event = FALSE;
        }
        break;
        }
        w = IntuitionBase->ActiveWindow;
    }
#endif
    if (gadget)
    {
        if (IS_BOOPSI_GADGET(gadget))
        {
        gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, reuse_event, IntuitionBase);
        }

    } /* if (there is an active gadget) */
    else if (w && (!req || req->Flags & NOISYREQ) && w != GetPrivScreen(w->WScreen)->MenuVerifyActiveWindow)
    {
        ih_fire_intuimessage(w,
                 IDCMP_MOUSEBUTTONS,
                 ie->ie_Code,
                 w,
                 IntuitionBase);
    }

    break; /* case MENUDOWN */

    case IECODE_NOBUTTON: /* MOUSEMOVE */
    {
        struct Screen *scr;
    UWORD DWidth, DHeight;

    if (ie->ie_Qualifier & IEQUALIFIER_RELATIVEMOUSE) {
        //ULONG Thresh;

        /* Add delta information lost in previous mousemove event. See below. */            
        iihdata->DeltaMouseX = ie->ie_X + iihdata->DeltaMouseX_Correction;
        iihdata->DeltaMouseY = ie->ie_Y + iihdata->DeltaMouseY_Correction;

#define ACCELERATOR_THRESH      2
#define ACCELERATOR_MULTI       2

        if (GetPrivIBase(IntuitionBase)->ActivePreferences->EnableCLI & MOUSE_ACCEL) {
        /* Acceleration */
        if (ABS(iihdata->DeltaMouseX) > ACCELERATOR_THRESH)
            iihdata->DeltaMouseX *= ACCELERATOR_MULTI;
        if (ABS(iihdata->DeltaMouseY) > ACCELERATOR_THRESH)
            iihdata->DeltaMouseY *= ACCELERATOR_MULTI;
        }
        
        switch(GetPrivIBase(IntuitionBase)->ActivePreferences->PointerTicks) {
        case 0:
        iihdata->DeltaMouseX_Correction = 0;
        iihdata->DeltaMouseX_Correction = 0;
        break;
            
        default:
        /* Remember the delta information which gets lost because of division by PointerTicks.
           Will be added to prescaled deltas of next mousemove event. If this is not done, moving
           the mouse very slowly would cause it to not move at all */
           
        iihdata->DeltaMouseX_Correction = iihdata->DeltaMouseX % GetPrivIBase(IntuitionBase)->ActivePreferences->PointerTicks;
        iihdata->DeltaMouseY_Correction = iihdata->DeltaMouseY % GetPrivIBase(IntuitionBase)->ActivePreferences->PointerTicks;
        
        iihdata->DeltaMouseX /= GetPrivIBase(IntuitionBase)->ActivePreferences->PointerTicks;
        iihdata->DeltaMouseY /= GetPrivIBase(IntuitionBase)->ActivePreferences->PointerTicks;
        break;
        }


        ie->ie_X = iihdata->DeltaMouseX + IntuitionBase->MouseX;
        ie->ie_Y = iihdata->DeltaMouseY + IntuitionBase->MouseY;
    } else {
        DEBUG_MOUSE(bug("[Inputhandler] Last mouse position: (%d, %d), new mouse position: (%d, %d)\n",
                IntuitionBase->MouseX, IntuitionBase->MouseY, ie->ie_X, ie->ie_Y));
        iihdata->DeltaMouseX = ie->ie_X - IntuitionBase->MouseX;
        iihdata->DeltaMouseY = ie->ie_Y - IntuitionBase->MouseY;
        DEBUG_MOUSE(bug("[InputHandler] Delta is (%d, %d)\n", iihdata->DeltaMouseX, iihdata->DeltaMouseY));
    }

    /* Calculate current display size.
       It's determined by the first screen on this monitor.
       TODO: perhaps we should just ask display driver about its current display mode? */
    scr = FindFirstScreen(GetPrivIBase(IntuitionBase)->ActiveMonitor, IntuitionBase);
    if (scr)
    {
        DWidth = scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MaxX - scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MinX + 1;
        DHeight = scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MaxY - scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MinY + 1;
    }
    else
    {
        /* If there's no active screen, we take 160x160 as a limit */
        DWidth = 160;
        DHeight = 160;
    }

    scr = iihdata->ScreenDrag;
    if (scr) {
        WORD dx = iihdata->DeltaMouseX;
        WORD dy = iihdata->DeltaMouseY;
        WORD min, max, val;
        UWORD spFlags = GetPrivScreen(scr)->SpecialFlags;
        UWORD DragMode = GetPrivIBase(IntuitionBase)->IControlPrefs.ic_VDragModes[0];

        DEBUG_DRAG(bug("[InputHandler] Screen drag, delta is (%d, %d)\n", dx, dy));

        /* Restrict dragging to a physical display area if the driver does not allow composition or if the user wants it*/
        if (((spFlags & SF_HorCompose) != SF_HorCompose) || (DragMode & ICVDM_HBOUND)) {
        /* Calculate limits */
        if (scr->Width > DWidth) {
            min = DWidth - scr->Width;
            max = 0;
        } else {
            min = 0;
            max = DWidth - scr->Width;
        }
        /* The purpose of the following complex check is to prevent jumping if the
           screen was positioned out of user drag limits by the program itself using
           ScreenPosition() or OpenScreen(). We apply restrictions in parts depending
           on the dragging direction.
           May be the user should also be able do drag the screen back off-display in such
           a case?
           Calculate the position we would go to */
        val = scr->LeftEdge + dx;
        /* Determine the direction */
        if ((dx < 0) && ((!(spFlags & SF_ComposeRight)) || (DragMode & ICVDM_LBOUND))) {
            /* Can we move at all in this direction ? */
            if (scr->LeftEdge > min) {
            /* If too far, restrict it */
            if (val < min)
                dx = min - scr->LeftEdge;
            } else
            /* Just don't move if we can't */
            dx = 0;
        } else if ((!(spFlags & SF_ComposeLeft)) || (DragMode & ICVDM_RBOUND)) {
            if (scr->LeftEdge < max) {
            if (val > max)
                dx = max - scr->LeftEdge;
            } else
            dx = 0;
        }
        }
        if (((spFlags & SF_VertCompose) != SF_VertCompose) || (DragMode & ICVDM_VBOUND)) {
        DEBUG_DRAG(bug("[Inputhandler] Restricting vertical drag\n"));
        DEBUG_DRAG(bug("[Inputhandler] Screen size: %d, display size: %d\n", scr->Height, DHeight));
        if (scr->Height > DHeight) {
            min = DHeight - scr->Height;
            max = 0;
        } else {
            min = 0;
            max = DHeight - scr->Height;
        }
        DEBUG_DRAG(bug("[Inputhandler] Limits: min %d max %d\n", min, max));
        val = scr->TopEdge + dy;
        DEBUG_DRAG(bug("[Inputhandler] New position would be %d\n", val));
        if ((dy < 0)  && ((!(spFlags & SF_ComposeBelow)) || (DragMode & ICVDM_TBOUND))) {
            if (scr->TopEdge > min) {
            if (val < min)
                dy = min - scr->TopEdge;
            } else
            dy = 0;
        } else if ((!(spFlags & SF_ComposeAbove)) || (DragMode & ICVDM_BBOUND)) {
            if (scr->TopEdge < max) {
            if (val > max)
                dy = max - scr->TopEdge;
            } else
            dy = 0;
        }
        DEBUG_DRAG(bug("[Inputhandler] Restricted delta will be %d\n", dy));
        }
        ScreenPosition(scr, SPOS_RELATIVE, dx, dy, 0, 0);
    }

    /* Autoscroll the active screen */
    scr = IntuitionBase->ActiveScreen;
    if (scr && (scr->Flags & AUTOSCROLL) &&
       (GetPrivScreen(scr)->MonitorObject == GetPrivIBase(IntuitionBase)->ActiveMonitor))
    {
        WORD xval = scr->LeftEdge;
        WORD yval = scr->TopEdge;
        WORD min;

        DEBUG_AUTOSCROLL(bug("[Inputhandler] Autoscroll screen 0x%p, event at (%d, %d)\n",
                     scr, ie->ie_X, ie->ie_Y));

        if ((ie->ie_X < 0) || (ie->ie_X >= DWidth)) {
        DEBUG_AUTOSCROLL(bug("[InputHandler] X delta: %d pixels\n", iihdata->DeltaMouseX));
        xval -= iihdata->DeltaMouseX;

        if (ie->ie_X < 0) {
            if (xval > 0)
            xval = 0;
        } else if (ie->ie_X >= DWidth) {
            min = DWidth - scr->Width;
            if (xval < min)
            xval = min;
        }
        }

        if ((ie->ie_Y < 0) || (ie->ie_Y >= DHeight)) {
            yval -= iihdata->DeltaMouseY;

            if (ie->ie_Y < 0) {
                /* If screen is dragged down and user touched upper screen boundry,
                do nothing */
                if (scr->TopEdge >= 0)
                    yval = scr->TopEdge;
                else
                    /* If scrolled down screen is beeing scrolled up, make sure it
                       does not go over 0 */
                    if (yval > 0)
                        yval = 0;
            } else if (ie->ie_Y >= DHeight) {
                min = DHeight - scr->Height;
                if (yval < min)
                    yval = min;
            }
        }

        if ((xval != scr->LeftEdge) || (yval != scr->TopEdge))
        ScreenPosition(scr, SPOS_ABSOLUTE, xval, yval, 0, 0);
    }

    /* Restrict mouse coordinates to the physical display area */
    if (ie->ie_X >= DWidth) ie->ie_X = DWidth - 1;
    if (ie->ie_Y >= DHeight) ie->ie_Y = DHeight - 1;
    if (ie->ie_X < 0) ie->ie_X = 0;
    if (ie->ie_Y < 0) ie->ie_Y = 0;

#ifdef SKINS
    if (gadget == iihdata->MasterDragGadget) {
        struct gpInput gpi;
        ULONG            retval;

        gpi.MethodID     = GM_MOVETEST;
        gpi.gpi_GInfo     = gi;
        gpi.gpi_Mouse.X = ie->ie_X - gi->gi_Window->WScreen->LeftEdge;
        gpi.gpi_Mouse.Y = ie->ie_Y - gi->gi_Window->WScreen->TopEdge;
        gpi.gpi_IEvent  = ie;

        retval = Locked_DoMethodA(gi->gi_Window, gadget, (Msg)&gpi, IntuitionBase);
        if (retval == MOVETEST_ADJUSTPOS)
        {
            ie->ie_X = gpi.gpi_Mouse.X + gi->gi_Window->WScreen->LeftEdge;
            ie->ie_Y = gpi.gpi_Mouse.Y + gi->gi_Window->WScreen->TopEdge;
        }

    }
#endif
    /* Do Mouse Bounding - mouse will be most restrictive of screen size or mouse bounds */
    if (iihdata->MouseBoundsActiveFlag) {            
        if (ie->ie_X < iihdata->MouseBoundsLeft)
        ie->ie_X = iihdata->MouseBoundsLeft;
        else if (ie->ie_X > iihdata->MouseBoundsRight)
        ie->ie_X = iihdata->MouseBoundsRight;
        
        if (ie->ie_Y < iihdata->MouseBoundsTop)
        ie->ie_Y = iihdata->MouseBoundsTop;
        else if (ie->ie_Y > iihdata->MouseBoundsBottom)
        ie->ie_Y = iihdata->MouseBoundsBottom;
    }

    IntuitionBase->MouseX = ie->ie_X;
    IntuitionBase->MouseY = ie->ie_Y;
    notify_mousemove_screensandwindows(IntuitionBase);
#if !SINGLE_SETPOINTERPOS_PER_EVENTLOOP
    MySetPointerPos(IntuitionBase);
#else
    *call_setpointerpos = TRUE;
#endif

    screen = FindActiveScreen(IntuitionBase); /* The mouse was moved, so current screen may have changed */

#ifdef SKINS
    if (!gadget) {
        if (iihdata->TitlebarOnTop)
        {
            if (screen->MouseY > screen->BarHeight && GetPrivScreen(screen)->SpecialFlags & SF_AppearingBar)
            {
            iihdata->TitlebarOnTop = FALSE;
            iihdata->TitlebarAppearTime = 0;

            LOCK_REFRESH(screen);

            MoveLayer(0, screen->BarLayer, 0, -(screen->BarHeight + 1));
            CheckLayers(screen, IntuitionBase);

            UNLOCK_REFRESH(screen);
            }
        }
        else
        {
            if (screen->MouseY == 0 && GetPrivScreen(screen)->SpecialFlags & SF_AppearingBar && !MENUS_ACTIVE && !(PeekQualifier() & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_RBUTTON|IEQUALIFIER_MIDBUTTON)))
            {
            if (!(iihdata->TitlebarAppearTime))
            {
                iihdata->TitlebarAppearTime = ((UQUAD)ie->ie_TimeStamp.tv_secs) * 50;
                iihdata->TitlebarAppearTime += ie->ie_TimeStamp.tv_micro / 20000;
            }
            }
            else
            {
            iihdata->TitlebarAppearTime = 0;
            }
        }
    }
#endif
    if (MENUS_ACTIVE) {
        FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
        *keep_event = FALSE;
        break;
    }

    if (gadget) {
        *keep_event = FALSE;

        switch (gadget->GadgetType & GTYP_GTYPEMASK)
        {
        case GTYP_BOOLGADGET:
        case 0: //fallback for sucky gadgets
            /* Must be a RELVERIFY gadget */
            {
            BOOL inside;

            inside = InsideGadget(gi->gi_Screen,
                          gi->gi_Window,
                          gi->gi_Requester,
                          gadget,
                          gi->gi_Screen->MouseX,
                          gi->gi_Screen->MouseY);

            if  (inside != iihdata->MouseWasInsideBoolGadget) {
                iihdata->MouseWasInsideBoolGadget = inside;

                gadget->Flags ^= GFLG_SELECTED;
                RefreshBoolGadgetState(gadget, w, req, IntuitionBase);
            }
            }
            break;

        case GTYP_PROPGADGET:
            HandlePropMouseMove(gadget,
                    w,
                    req,
                    w->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gadget, gi->gi_Screen, gi->gi_Window, NULL),
                    w->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gadget, gi->gi_Screen, gi->gi_Window, NULL),
                    IntuitionBase);

            break;

        case GTYP_CUSTOMGADGET:
            gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, reuse_event, IntuitionBase);
            break;

        } /* switch GadgetType */

    } /* if (a gadget is currently active) */

    *keep_event = FALSE;

    if (!w)
        break;

    if (IW(w)->helpflags & HELPF_GADGETHELP && (!(PeekQualifier() & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_RBUTTON|IEQUALIFIER_MIDBUTTON)))) {
        struct Window *hw;
        struct Gadget *g;

        hw = FindActiveWindow(ie, screen, 0, IntuitionBase);

        if (hw != w &&
            (!hw || !(IW(w)->helpflags & HELPF_ISHELPGROUP) ||
             !(IW(hw)->helpflags & HELPF_ISHELPGROUP) ||
             IW(w)->helpgroup != IW(hw)->helpgroup))
        {

            if (iihdata->LastHelpWindow)
            {
            fire_intuimessage(w,
                      IDCMP_GADGETHELP,
                      0,
                      NULL,
                      IntuitionBase);

            iihdata->LastHelpGadget = NULL;
            iihdata->LastHelpWindow = NULL;
            iihdata->HelpGadgetFindTime = 0;
            }
        }
        else
        {
            g = FindHelpGadget (hw,
                    IntuitionBase->ActiveScreen->MouseX,
                    IntuitionBase->ActiveScreen->MouseY,
                    IntuitionBase);
            if (g && g != iihdata->LastHelpGadget)
            {
            if (!iihdata->LastHelpGadget)
            {
                iihdata->HelpGadgetFindTime = ((UQUAD)ie->ie_TimeStamp.tv_secs) * 50;
                iihdata->HelpGadgetFindTime += ie->ie_TimeStamp.tv_micro / 20000;
            } else {
                if (hw == iihdata->LastHelpWindow)
                {
                iihdata->HelpGadgetFindTime = ((UQUAD)ie->ie_TimeStamp.tv_secs) * 50;
                iihdata->HelpGadgetFindTime += ie->ie_TimeStamp.tv_micro / 20000;
                iihdata->HelpGadgetFindTime += 25;//smaller delay
                }
            }
            }
            else if (g != iihdata->LastHelpGadget ||
             hw != iihdata->LastHelpWindow)
            {
            fire_intuimessage(hw,
                      IDCMP_GADGETHELP,
                      0, /* Don't know what it should be */
                      hw,
                      IntuitionBase);
            }

            iihdata->LastHelpGadget = g;
            iihdata->LastHelpWindow = hw;
        }
    } else {
        iihdata->LastHelpGadget = NULL;
        iihdata->LastHelpWindow = NULL;
        iihdata->HelpGadgetFindTime = 0;
    }

    if (!(w->IDCMPFlags & IDCMP_MOUSEMOVE))
        break;

    /* Send IDCMP_MOUSEMOVE if WFLG_REPORTMOUSE is set
       and/or active gadget has GACT_FOLLOWMOUSE set */

    /* jDc: do NOT send when sizegad is pressed */
    if (!(w->Flags & WFLG_REPORTMOUSE)) {
        if (!gadget)
            break;
        if (!(gadget->Activation & GACT_FOLLOWMOUSE))
            break;
    } else {
        if (gadget && (gadget->GadgetType & (GTYP_SIZING|GTYP_WDRAGGING)))
            break;
    }

    orig_ie->ie_Class = IECLASS_RAWMOUSE;

    /* Limit the number of IDCMP_MOUSEMOVE messages sent to intuition.
       note that this comes after handling gadgets, because gadgets should get all events.
    */

    if (IW(w)->num_mouseevents >= IW(w)->mousequeue) {
        BOOL old_msg_found = FALSE;

        /* Mouse Queue is full, so try looking for a not
           yet GetMsg()ed IntuiMessage in w->UserPort
           trying to modify that. */

        Forbid();
        if (w->UserPort)
        {
            struct IntuiMessage *im;

            for (im = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_TailPred;
            im->ExecMessage.mn_Node.ln_Pred;
            im = (struct IntuiMessage *)im->ExecMessage.mn_Node.ln_Pred)
            {
            if ((im->Class == IDCMP_MOUSEMOVE) &&
                (im->IDCMPWindow == w))
            {
                im->Qualifier = iihdata->ActQualifier;

                if (w->IDCMPFlags & IDCMP_DELTAMOVE)
                {
                im->MouseX = iihdata->DeltaMouseX;
                im->MouseY = iihdata->DeltaMouseY;
                }
                else
                {
                im->MouseX = w->MouseX;
                im->MouseY = w->MouseY;
                }
                CurrentTime(&im->Seconds, &im->Micros);

                old_msg_found = TRUE;
                break;
            }
            }
        } /* if (w->UserPort) */
        Permit();

        /* no need to send a new message if we modified
           an existing one ... */

        if (old_msg_found) break;

        /* ... otherwise we are in a strange situation. The mouse
           queue is full, but we did not find an existing MOUSEMOVE
           imsg in w->UserPort. So the app probably has removed
           an imsg from the UserPort with GetMsg but we did not get
           the ReplyMsg, yet. In this case we do send a new message */

        HandleIntuiReplyPort(iihdata, IntuitionBase);

    }

    /* MouseQueue is not full, so we can send a message. We increase
       IntWindow->num_mouseevents which will later be decreased after
       the Intuition InputHandler gets the ReplyMessage from the app
       and handles it in HandleIntuiReplyPort() */

    if (ih_fire_intuimessage(w, IDCMP_MOUSEMOVE, IECODE_NOBUTTON, w, IntuitionBase))
        IW(w)->num_mouseevents++;

    break;

    } /* case IECODE_NOBUTTON */
    } /* switch (ie->ie_Code)  (what button was pressed ?) */
    
    return gadget;
}

/****************************************************************************************/

AROS_UFH2(struct InputEvent *, IntuiInputHandler,
          AROS_UFHA(struct InputEvent *,      oldchain,       A0),
          AROS_UFHA(struct IIHData *,         iihdata,        A1)
         )
{
    AROS_USERFUNC_INIT

    struct InputEvent     *ie, *orig_ie, *next_ie, stackie;
    struct Gadget         *gadget = NULL;
    struct IntuitionBase  *IntuitionBase = iihdata->IntuitionBase;
    struct Library        *KeymapBase = GetPrivIBase(IntuitionBase)->KeymapBase;
    struct Screen       *screen;
    //ULONG                lock;
    struct GadgetInfo     *gi = &iihdata->GadgetInfo;
    BOOL                   reuse_event, ie_used;
    struct Window         *w;
    struct Requester      *req;
    ULONG                  stitlebarhit = 0;
#if 0 /* Toolbox is broken-as-designed */
    struct Window         *toolbox;
    struct GadgetInfo     *boxgi = &iihdata->BoxGadgetInfo;
    struct Gadget         *boxgadget = NULL;
#endif
#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
    BOOL                   call_setpointerpos = FALSE;
#endif

    D(bug("Inside intuition inputhandler, active window=%p\n", IntuitionBase->ActiveWindow));
    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);

    if (!iihdata->InputDeviceTask) iihdata->InputDeviceTask = FindTask(NULL);

    /* Then free generated InputEvents done in the previous round */

    FreeGeneratedInputEvents(iihdata);

    /* First handle IntuiMessages which were replied back to the IntuiReplyPort
       by the apps */

    HandleIntuiReplyPort(iihdata, IntuitionBase);

    /* Handle action messages */

    HandleIntuiActions(iihdata, IntuitionBase);

    /* Now handle the input events */

    ie = &stackie;
    reuse_event = FALSE;
    next_ie = oldchain;

    /* shut up the compiler */
    orig_ie = next_ie;
    ie_used = FALSE;

    gadget = iihdata->ActiveGadget;

    while (reuse_event || next_ie)
    {
        struct Window   *old_w;
        BOOL             keep_event = TRUE;
        BOOL             new_active_window = FALSE;
    Object *newmonitor = GetPrivIBase(IntuitionBase)->NewMonitor;

    /* Process hosted display activation event (if any).
       This is experimental. If this works badly, we'll possibly have to put it into
       input events queue */
    if (newmonitor) {
        DEBUG_MONITOR(bug("[Inputhandler] Activating monitor 0x%p\n", newmonitor));
        GetPrivIBase(IntuitionBase)->NewMonitor = NULL;
        ActivateMonitor(newmonitor, -1, -1, IntuitionBase);
        iihdata->SwitchedMonitor = TRUE;
    }

        if (!reuse_event)
        {
            *ie = *next_ie;
            orig_ie = next_ie;
            next_ie = ie->ie_NextEvent;
            ie_used = FALSE;
        }

        D(bug("iih: Handling event of class %d, code %d\n", ie->ie_Class, ie->ie_Code));
        reuse_event = FALSE;

    /* If the monitor has been changed, this possibly happened because of mouse click in
       its display window. In such a case we have to update current mouse coordinates
       from the first absolute mouse event. Otherwise input will misbehave. */
    if (iihdata->SwitchedMonitor && (ie->ie_Class == IECLASS_RAWMOUSE)) {
        iihdata->SwitchedMonitor = FALSE;
         if (!(ie->ie_Qualifier & IEQUALIFIER_RELATIVEMOUSE)) {
        DEBUG_MONITOR(bug("[Inputhandler] Adjusting coordinates to (%d, %d)\n", ie->ie_X, ie->ie_Y));
        IntuitionBase->MouseX = ie->ie_X;
        IntuitionBase->MouseY = ie->ie_Y;
        notify_mousemove_screensandwindows(IntuitionBase);
        }
    }

        /* new event, we need to reset this */
        screen = FindActiveScreen(IntuitionBase);
        iihdata->ActEventTablet = 0;

        /* Set the timestamp in IntuitionBase */

        IntuitionBase->Seconds = ie->ie_TimeStamp.tv_secs;
        IntuitionBase->Micros  = ie->ie_TimeStamp.tv_micro;

#if 0 /* toolbox stuff disabled. broken. calling LockLayerinfo() for every event is broken. deadlocks */
        /* Use event to find the active window */

        toolbox = GetToolBoxWindow(ie, screen, IntuitionBase);

        if (toolbox)
        {
            /* Do ToolBox Window Actions */
            /* ToolBox Windows supports only a subset of IECLASS Actions */
            switch (ie->ie_Class) {

            case IECLASS_RAWMOUSE:
            boxgadget = Process_RawMouse(ie, iihdata, screen, toolbox, boxgadget, boxgi, 0, FALSE, TRUE,
                         orig_ie, &keep_event, &reuse_event,
#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
                         &call_setpointerpos,
#endif
                         IntuitionBase);
        break;

            } /* switch (ie->ie_Class) */
        } /* if (toolbox) */
#endif

        w = IntuitionBase->ActiveWindow;

        if (!MENUS_ACTIVE && !SYSGADGET_ACTIVE)
        {
            /* lock = LockIBase(0UL); */

            old_w = w;
            if (ie->ie_Class == IECLASS_RAWMOUSE && ie->ie_Code == SELECTDOWN)
            {
                w = FindActiveWindow(ie, screen, &stitlebarhit, IntuitionBase);
                DEBUG_CLICK(bug("iih:New active window: %p\n", w));
            }


            if ( w != old_w )
            {
                if (w)
                {
                    DEBUG_WINDOW(bug("Activating new window (title %s)\n", w->Title ? w->Title : "<noname>"));

                    DEBUG_WINDOW(bug("Window activated\n"));
                }
                else
                {
                    DEBUG_WINDOW(bug("Making active window inactive. Now there's no active window\n"));
                }
                new_active_window = TRUE;
                iihdata->NewActWindow = w;
            }

            /* UnlockIBase(lock); */

            if (new_active_window)
            {
                if (gadget && w &&
                    (!(GetPrivScreen(w->WScreen)->MenuVerifyMsgCount)) &&
                    (!(MENUS_ACTIVE)) && (!(SYSGADGET_ACTIVE)))
                {
                    switch (gadget->GadgetType & GTYP_GTYPEMASK)
                    {

                    case GTYP_CUSTOMGADGET:
                        {
                            struct gpGoInactive gpgi;

                            gpgi.MethodID   = GM_GOINACTIVE;
                            gpgi.gpgi_GInfo = gi;
                            gpgi.gpgi_Abort = 1;

                            Locked_DoMethodA(gi->gi_Window, gadget, (Msg)&gpgi, IntuitionBase);
                            break;
                        }

                    case GTYP_STRGADGET:
                            gadget->Flags &= ~GFLG_SELECTED;
                            RefreshStrGadget(gadget, gi->gi_Window, gi->gi_Requester, IntuitionBase);
                            break;

                    case GTYP_BOOLGADGET:
                            /* That a bool gadget is active here can only happen
                               if user used LMB to activate gadget and LAMIGA + LALT
                               to activate other window, or viceversa */
                            /* The gadget must be a RELVERIFY one */
                            if (!(gadget->Activation & GACT_TOGGLESELECT))
                            {
                            BOOL inside;

                            inside = InsideGadget(gi->gi_Screen, gi->gi_Window,
                                              gi->gi_Requester, gadget,
                                              gi->gi_Screen->MouseX, gi->gi_Screen->MouseY);

                            if (inside)
                            {
                                    gadget->Flags &= ~GFLG_SELECTED;
                                    RefreshBoolGadgetState(gadget, gi->gi_Window,
                                                       gi->gi_Requester, IntuitionBase);
                            }
                            }
                            break;

                    case GTYP_PROPGADGET:
                            /* That a prop gadget is active here can only happen
                               if user used LMB to activate gadget and LAMIGA + LALT
                               to activate other window, or viceversa */

                            HandlePropSelectUp(gadget, gi->gi_Window, NULL, IntuitionBase);
                            if (gadget->Activation & GACT_RELVERIFY)
                            {
                            ih_fire_intuimessage(gi->gi_Window,
                                                 IDCMP_GADGETUP,
                                                 0,
                                                 gadget,
                                                 IntuitionBase);
                            }
                            break;
                
                    } /* switch (gadget->GadgetType & GTYP_GTYPEMASK) */

                    gadget->Activation &= ~GACT_ACTIVEGADGET;
                    iihdata->ActiveGadget = NULL;
                    gadget = NULL;
                }

                /* ActivateWindow works if w = NULL */
                /* jacaDcaps: some gui toolkits (die reaction, die!) close the window opened by a boopsi gadget when
                   it gets hit with lmb, so we need to check if the new active window does not go away by
                   performing GM_GOINACTIVE on the gadget. NOTE: CloseWindow's part performed on input.device context
                   clears the iihdata->NewActWindow if it's the actually closed one. */
                if (w == iihdata->NewActWindow)
                {
                    ActivateWindow(w);
                }
        else
        {
                    w = IntuitionBase->ActiveWindow;
                    new_active_window = FALSE;
                    ie->ie_Class = IECLASS_NULL; //lose the event, otherwise the gadget will get activated again ;)
                }

                iihdata->NewActWindow = 0;

            } /* if (new_active_window) */

        } /* if (!MENUS_ACTIVE) */

        req = NULL;
        if (w)
        {
            req = w->FirstRequest;
        }

        D(bug("[Inputhandler] Screen 0x%p Window 0x%p Requester 0x%p gadget 0x%p\n", screen, w, req, gadget));

        switch (ie->ie_Class) {
        case IECLASS_POINTERPOS:
            ie->ie_SubClass = IESUBCLASS_COMPATIBLE;
            /* fall through */

        case IECLASS_NEWPOINTERPOS:
            switch (ie->ie_SubClass)
            {
            case IESUBCLASS_COMPATIBLE:
                ie->ie_Code = IECODE_NOBUTTON;
                break;

            case IESUBCLASS_PIXEL:
                {
                    struct IEPointerPixel *pp = ie->ie_EventAddress;

                    ie->ie_X = pp->iepp_Position.X + pp->iepp_Screen->LeftEdge;
                    ie->ie_Y = pp->iepp_Position.Y + pp->iepp_Screen->TopEdge;

            ActivateMonitor(GetPrivScreen(pp->iepp_Screen)->MonitorObject, ie->ie_X, ie->ie_Y, IntuitionBase);
                }
                ie->ie_Code = IECODE_NOBUTTON;
                break;

            case IESUBCLASS_TABLET:
                {
                //unsupported - does anything use it anyway? ;)
                }
                ie->ie_Code = 0;
                break;

            case IESUBCLASS_NEWTABLET:
                {
                    struct IENewTablet *nt = (struct IENewTablet *)ie->ie_EventAddress;

                    if (nt)
                    {
                        iihdata->ActEventTablet = nt; //cache this
                        ie->ie_X = (screen->Width * nt->ient_TabletX) / nt->ient_RangeX;
                        ie->ie_Y = (screen->Height * nt->ient_TabletY) / nt->ient_RangeY;
                    }
                    ie->ie_Class = IECLASS_RAWMOUSE;
                }
                break;

            default:
                ie->ie_Code = 0;
                break;
            }
            /* fall through */

        case IECLASS_RAWMOUSE:
        gadget = Process_RawMouse(ie, iihdata, screen, w, gadget, gi, stitlebarhit, new_active_window, FALSE,
                      orig_ie, &keep_event, &reuse_event,
#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
                      &call_setpointerpos,
#endif
                      IntuitionBase);
        keep_event = TRUE;
            break;

        case IECLASS_RAWKEY:
            /* release events go only to gadgets and windows who
               have not set IDCMP_VANILLAKEY */

            DEBUG_HANDLER(dprintf("Handler: IECLASS_RAWKEY\n"));
            DEBUG_KEY(dprintf("Handler: Qual 0x%lx\n",iihdata->ActQualifier));

            iihdata->ActQualifier &= ~(KEY_QUALIFIERS | IEQUALIFIER_REPEAT);
            iihdata->ActQualifier |= (ie->ie_Qualifier & (KEY_QUALIFIERS | IEQUALIFIER_REPEAT));

            DEBUG_KEY(dprintf("Handler: real Qual 0x%lx\n",iihdata->ActQualifier));

            /* Keyboard mouse emulation and screen switching */

            {
                UWORD code = ie->ie_Code & ~IECODE_UP_PREFIX;
                DEBUG_KEY(dprintf("Handler: code 0x%lx\n",code));

                /* Left Amiga + N/M screen switching shortcut */
                if ((ie->ie_Qualifier & IEQUALIFIER_LCOMMAND) && (code == RAWKEY_N || code == RAWKEY_M)) {
                    if (!(ie->ie_Qualifier & IEQUALIFIER_REPEAT) && !(ie->ie_Code & IECODE_UP_PREFIX)) {
                        if (code == RAWKEY_N)
                            WBenchToFront();
                        else if (code == RAWKEY_M)
                            ScreenToBack(IntuitionBase->FirstScreen);
                    }
                    keep_event = FALSE;
                    break;
                }

                /* Mouse button emulation: LALT + LAMIGA = LBUTTON, RALT + RAMIGA = RBUTTON */
                if ((code == RAWKEY_LAMIGA) ||
                    (code == RAWKEY_LALT)   ||
                    (code == RAWKEY_RAMIGA) ||
                    (code == RAWKEY_RALT))
                {
                    DEBUG_KEY(dprintf("Handler: KeyMouseEmul\n"));
                    iihdata->PrevKeyMouseState = iihdata->ActKeyMouseState;
                    iihdata->ActKeyMouseState = 0;
                    if ((ie->ie_Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_LALT)) == (IEQUALIFIER_LCOMMAND | IEQUALIFIER_LALT))
                    {
                        iihdata->ActKeyMouseState |= IEQUALIFIER_LEFTBUTTON;
                    }
                    if ((ie->ie_Qualifier & (IEQUALIFIER_RCOMMAND | IEQUALIFIER_RALT)) == (IEQUALIFIER_RCOMMAND | IEQUALIFIER_RALT))
                    {
                        iihdata->ActKeyMouseState |= IEQUALIFIER_RBUTTON;
                    }

                    if ((iihdata->ActKeyMouseState & IEQUALIFIER_LEFTBUTTON) != (iihdata->PrevKeyMouseState & IEQUALIFIER_LEFTBUTTON))
                    {
                        orig_ie->ie_Class    = IECLASS_RAWMOUSE;
                        orig_ie->ie_SubClass = 0;
                        orig_ie->ie_Code     = (iihdata->ActKeyMouseState & IEQUALIFIER_LEFTBUTTON) ? IECODE_LBUTTON : IECODE_LBUTTON | IECODE_UP_PREFIX;
                        orig_ie->ie_X        = 0;
                        orig_ie->ie_Y        = 0;
                        *ie = *orig_ie;

                        reuse_event = TRUE;
                        break;
                    }

                    if ((iihdata->ActKeyMouseState & IEQUALIFIER_RBUTTON) != (iihdata->PrevKeyMouseState & IEQUALIFIER_RBUTTON))
                    {
                        orig_ie->ie_Class    = IECLASS_RAWMOUSE;
                        orig_ie->ie_SubClass = 0;
                        orig_ie->ie_Code     = (iihdata->ActKeyMouseState & IEQUALIFIER_RBUTTON) ? IECODE_RBUTTON : IECODE_RBUTTON | IECODE_UP_PREFIX;
                        orig_ie->ie_X        = 0;
                        orig_ie->ie_Y        = 0;
                        *ie = *orig_ie;

                        reuse_event = TRUE;
                        break;
                    }

                } /* if key is one of LAMIGA/LALT/RAMIGA/RALT */

                if ((iihdata->ActQualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) &&
                    ((ie->ie_Code == RAWKEY_UP)    ||
                     (ie->ie_Code == RAWKEY_DOWN)  ||
                     (ie->ie_Code == RAWKEY_LEFT)  ||
                     (ie->ie_Code == RAWKEY_RIGHT)))
                {
                    static BYTE const xmap[] = { 0, 0, 1, -1};
                    static BYTE const ymap[] = {-1, 1, 0,  0};
                    WORD        shift;

                    shift = (iihdata->ActQualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? 40 : 1;

                    /* Mouse Move Emulation */

                    orig_ie->ie_Class     = IECLASS_RAWMOUSE;
                    orig_ie->ie_SubClass  = 0;
                    orig_ie->ie_Code      = IECODE_NOBUTTON;
                    orig_ie->ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
                    orig_ie->ie_X         = xmap[code - RAWKEY_UP] * shift;
                    orig_ie->ie_Y         = ymap[code - RAWKEY_UP] * shift;

                    *ie = *orig_ie;
                    reuse_event = TRUE;
                    break;
                }

            } /**/

            /* End Keyboard mouse emulation */

            if (MENUS_ACTIVE)
            {
                DEBUG_KEY(dprintf("Handler: FireMenuMessage\n"));
                FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
                keep_event = FALSE;
                break;
            }

            /* Hotkeys processing */
#ifdef SKINS
            {
                ULONG result;
        
                if (!(ie->ie_Code & IECODE_UP_PREFIX))
                if ((result = RunHotkeys(ie,IntuitionBase)))
                {
                    if (result == RUNHOTREUSE)
                    {
                        reuse_event = TRUE;
                    }
            else
            {
                        keep_event = FALSE;
                    }
                    break;
                }
                w = IntuitionBase->ActiveWindow;
            }
#endif

            if ( keep_event &&
                ((!(ie->ie_Code & IECODE_UP_PREFIX)) ||
                 gadget ||
                 (w && ((w->IDCMPFlags & IDCMP_VANILLAKEY) == 0)) ))
            {
                if (gadget)
                {
                    keep_event = FALSE;

                    DEBUG_KEY(dprintf("Handler: Gadget 0x%lx active\n",gadget));
                    DEBUG_KEY(dprintf("Handler: GadgetID 0x%lx UserData 0x%lx\n",
                              gadget->GadgetID,
                              gadget->UserData));
                    DEBUG_KEY(dprintf("Handler: GadgetType 0x%lx Flags 0x%lx Activation 0x%lx\n",
                              gadget->GadgetType,
                              gadget->Flags,
                              gadget->Activation));
                    DEBUG_KEY(dprintf("Handler: MoreFlags 0x%lx\n",
                              ((struct ExtGadget*)gadget)->MoreFlags));

                    switch (gadget->GadgetType & GTYP_GTYPEMASK)
                    {
                    case GTYP_STRGADGET:
                        {
                            UWORD imsgcode;
                            ULONG ret = HandleStrInput(gadget, gi, ie, &imsgcode,
                                           IntuitionBase);

                            DEBUG_KEY(dprintf("Handler: Key GTYP_STRGADGET ret 0x%lx\n",ret));
                            if (ret & (SGA_END | SGA_NEXTACTIVE | SGA_PREVACTIVE))
                            {
                                if (gadget->Activation & GACT_RELVERIFY)
                                {
                                    DEBUG_KEY(dprintf("Handler: GACT_RELVERIFY\n"));
                                    ih_fire_intuimessage(w,
                                                     IDCMP_GADGETUP,
                                                     imsgcode,
                                                     gadget,
                                                     IntuitionBase);

                                    if (req && gadget->Activation & GACT_ENDGADGET)
                                    {
                                        DEBUG_KEY(dprintf("Handler: GACT_ENDGADGET\n"));
                                        EndRequest(req, w);

                                        req = w->FirstRequest;
                                        ret = 0;
                                    }
                                }

                                if ((gadget->Flags & GFLG_TABCYCLE) && (ret & SGA_NEXTACTIVE))
                                {
                                    gadget = FindCycleGadget(w, req, gadget, GMR_NEXTACTIVE);
                                    DEBUG_KEY(dprintf("Handler: TabCycle next gadget 0x%lx\n",gadget));
                                }
                                else if ((gadget->Flags & GFLG_TABCYCLE) && (ret & SGA_PREVACTIVE))
                                {
                                    gadget = FindCycleGadget(w, req, gadget, GMR_PREVACTIVE);
                                    DEBUG_KEY(dprintf("Handler: TabCycle prev gadget 0x%lx\n",gadget));
                                }
                                else
                                {
                                    gadget = NULL;
                                }

                                if (gadget)
                                {
                                    gadget = DoActivateGadget(w, req, gadget, IntuitionBase);
                                }

                            } /* if (ret & (SGA_END | SGA_NEXTACTIVE | SGA_PREVACTIVE)) */

                            break;
                        }

                    case GTYP_CUSTOMGADGET:
                        DEBUG_KEY(dprintf("Handler: GTYP_CUSTOMGADGET\n"));
                        DEBUG_KEY(dprintf("Handler: send GM_HANDLEINPUT\n"));
                        gadget = DoGPInput(gi,
                                       gadget,
                                       ie,
                                       GM_HANDLEINPUT,
                                       &reuse_event,
                                       IntuitionBase);
                        DEBUG_KEY(dprintf("Handler: reuse %ld\n",reuse_event));
                        break;

                    } /* switch (gadget type) */

                } /* if (a gadget is currently active) */
                else if (w && (!req || req->Flags & NOISYREQ))
                {
                    BOOL menushortcut = FALSE;

                    DEBUG_KEY(dprintf("Handler: No Gadget active\n"));
                    DEBUG_KEY(dprintf("Handler: Qualifier 0x%lx WinFlags 0x%lx IDCMP 0x%lx\n",ie->ie_Qualifier,w->Flags,w->IDCMPFlags));

                    if ((ie->ie_Qualifier & IEQUALIFIER_RCOMMAND) &&
                        (!(w->Flags & WFLG_RMBTRAP)) &&
                        (w->IDCMPFlags & IDCMP_MENUPICK))
                    {
                        struct Menu *strip = 0;

                        DEBUG_KEY(dprintf("Handler: MenuKey\n"));
                        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

                        strip = w->MenuStrip;

                        if (((struct IntWindow *)w)->menulendwindow)
                        {
                            strip = ((struct IntWindow *)w)->menulendwindow->MenuStrip;
                        }

                        DEBUG_KEY(dprintf("Handler: MenuStrip 0x%lx\n",strip));
                        if (strip)
                        {
                            UBYTE key;

                            if (MapRawKey(ie, &key, 1, NULL) == 1)
                            {
                                UWORD menucode;

                                menucode = FindMenuShortCut(strip, key, TRUE, IntuitionBase);

                                DEBUG_KEY(dprintf("Handler: menucode 0x%lx\n",menucode));

                                if (menucode != MENUNULL)
                                {
                                    DEBUG_KEY(dprintf("Handler: build menuevent\n"));
                                    ie->ie_Class        = IECLASS_MENU;
                                    ie->ie_SubClass     = IESUBCLASS_MENUSTOP;
                                    ie->ie_EventAddress = w;
                                    ie->ie_Code         = menucode;

                                    reuse_event = TRUE;
                                    menushortcut = TRUE;

                                    MENUS_ACTIVE = TRUE;
                                    iihdata->MenuWindow = w;
                                }
                            }
                            else
                            {
                                DEBUG_KEY(dprintf("Handler: MapRawKey failed\n"));
                            }
                        }
                        if (!menushortcut) /* !! */
                            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

                    } /* if could be a menu short cut */
                    else
                        if ((ie->ie_Qualifier & IEQUALIFIER_RCOMMAND) &&
                            (!(w->IDCMPFlags & IDCMP_MENUPICK)))
                        {
                            struct Menu   *strip = 0;
                            struct Window *window = w;

                            /* not sure here about RMBTRAP */
                            DEBUG_KEY(dprintf("Handler: no idcmp, create a MENULIST idcmp\n"));

                            ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

                            strip = w->MenuStrip;

                            if (((struct IntWindow *)w)->menulendwindow)
                            {
                                strip = ((struct IntWindow *)w)->menulendwindow->MenuStrip;
                                window = ((struct IntWindow *)w)->menulendwindow;
                            }

                            DEBUG_KEY(dprintf("Handler: MenuStrip 0x%lx\n",strip));
                            if (strip)
                            {
                                UBYTE key;

                                if (MapRawKey(ie, &key, 1, NULL) == 1)
                                {
                                    UWORD menucode;

                                    menucode = FindMenuShortCut(strip, key, TRUE, IntuitionBase);

                                    DEBUG_KEY(dprintf("Handler: menucode 0x%lx\n",menucode));

                                    if (menucode != MENUNULL)
                                    {
                                        DEBUG_KEY(dprintf("Handler: build menuevent\n"));
                                        ih_fire_intuimessage(window,
                                                         IDCMP_MENUPICK,
                                                         menucode,
                                                         ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machines!? */
                                                         IntuitionBase);
                                        keep_event = FALSE;
                                        menushortcut = TRUE;
                                    }
                                }
                                else
                                {
                                    DEBUG_KEY(dprintf("Handler: MapRawKey failed\n"));
                                }
                            }
                            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
                        } /* if could be a menu short but current window has no idcmp cut */

                    if (menushortcut)
                    {
                        DEBUG_KEY(dprintf("Handler: menu shortcut..break\n"));
                        break;
                    }

                    /* This is a regular RAWKEY event (no gadget taking care
                       of it...). */

                    if (iihdata->ActQualifier & IEQUALIFIER_REPEAT)
                    {
                        /* don't send repeat key events if repeatqueue is full */
                        if (IW(w)->num_repeatevents >= IW(w)->repeatqueue)
                        {
                            DEBUG_KEY(dprintf("Handler: RepeatEvents full..don't send more\n"));
                            break;
                        }
                    }

                    if (w->IDCMPFlags & IDCMP_VANILLAKEY)
                    {
                        UBYTE keyBuffer;

                        DEBUG_KEY(dprintf("Handler: VANILLAKEY\n"));
                        //              DEBUG_KEY(dprintf("Handler: MapRawKey ie 0x%lx KeyMapBase 0x%lx IntutionBase 0x%lx\n",ie,KeymapBase,IntuitionBase));

                        if (MapRawKey(ie, &keyBuffer, 1, NULL) == 1)
                        {
                            DEBUG_KEY(dprintf("Handler: send VANILLAKEY msg\n"));
                            ih_fire_intuimessage(w,
                                         IDCMP_VANILLAKEY,
                                         keyBuffer,
                                         ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machines!? */
                                         IntuitionBase);
                            DEBUG_KEY(dprintf("Handler: done\n"));
                            break;
                        }

                        /* If the event mapped to more than one byte, it is not
                           a legal VANILLAKEY, so we send it as the original
                           RAWKEY event. */

                    }

                    if (w->IDCMPFlags & IDCMP_RAWKEY)
                    {
                        DEBUG_KEY(dprintf("Handler: send IDCMP_RAWKEY Qual 0x%lx Code 0x%lx addr 0x%lx Event\n",
                                  ie->ie_Qualifier,ie->ie_Code,ie->ie_position.ie_addr));
                        ih_fire_intuimessage(w,
                                     IDCMP_RAWKEY,
                                     ie->ie_Code,
                                     ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machine!? */
                                     IntuitionBase);
                        keep_event = FALSE;
                    }

                    DEBUG_KEY(dprintf("Handler: done\n"));
                } /* regular RAWKEY */
            }

            break; /* case IECLASS_RAWKEY */

        case IECLASS_TIMER:
        if (iihdata->MouseBoundsKillTimer)
        {
        iihdata->MouseBoundsKillTimer--;
        if (iihdata->MouseBoundsKillTimer == 0)
        {
            iihdata->MouseBoundsActiveFlag = FALSE;
        }
        }

            if (GetPrivIBase(IntuitionBase)->PointerDelay)
            {
                ULONG lock = LockIBase(0);

                if (--GetPrivIBase(IntuitionBase)->PointerDelay == 0)
                {
                    struct SharedPointer    *shared_pointer;
                    struct Window           *window = IntuitionBase->ActiveWindow;
                    struct IntScreen         *scr;
                    Object                  *pointer = ((struct IntWindow *)window)->pointer;

                    DEBUG_POINTER(dprintf("InputHandler: PointerDelay\n"));
                    DEBUG_POINTER(dprintf("InputHandler:  Pointer 0x%lx\n",
                                  pointer));

                    if (window)
                    {
                        DEBUG_POINTER(dprintf("InputHandler:  Window 0x%lx\n",
                                      window));
                        scr = GetPrivScreen(window->WScreen);
                        if (scr)
                        {
                            DEBUG_POINTER(dprintf("InputHandler:  Screen 0x%lx\n",
                                          scr));
                            if (pointer == NULL)
                            {
                                pointer = GetPrivIBase(IntuitionBase)->DefaultPointer;
                            }

                            if (((struct IntWindow *)window)->busy)
                            {
                                pointer = GetPrivIBase(IntuitionBase)->BusyPointer;
                            }

                            GetAttr(POINTERA_SharedPointer, pointer, (IPTR *) &shared_pointer);

                            DEBUG_POINTER(dprintf("InputHandler: scr 0x%lx pointer 0x%lx shared_pointer 0x%lx\n",
                                          scr, pointer, shared_pointer));
                            DEBUG_POINTER(dprintf("InputHandler: sprite 0x%lx\n",
                                          shared_pointer->sprite));

                            if (DoMethod(scr->MonitorObject, MM_SetPointerShape, shared_pointer))
                            {
                                ObtainSharedPointer(shared_pointer, IntuitionBase);
                                ReleaseSharedPointer(scr->Pointer, IntuitionBase);
                                scr->Pointer = shared_pointer;
                                if (window)
                                {
                                    window->XOffset = shared_pointer->xoffset;
                                    window->YOffset = shared_pointer->yoffset;
                                }
                            }
                            else
                            {
                                DEBUG_POINTER(dprintf("InputHandler: can't set pointer.\n"));
                            }
                        }
                        else
                        {
                            DEBUG_POINTER(dprintf("InputHandler: no screen.\n"));
                        }
                    }
                    else
                    {
                        DEBUG_POINTER(dprintf("InputHandler: no window.\n"));
                    }
                }

                UnlockIBase(lock);
            }

            if (GetPrivIBase(IntuitionBase)->MenuVerifyScreen)
            {
                struct IntScreen *scr = GetPrivIBase(IntuitionBase)->MenuVerifyScreen;
        
                if ((--scr->MenuVerifyTimeOut) <= 0)
                {
                    struct InputEvent ie;

                    /* currently we ONLY need the menu open time ! */
                    ie.ie_TimeStamp.tv_secs = IntuitionBase->Seconds;
                    ie.ie_TimeStamp.tv_micro = IntuitionBase->Micros;

                    if (FireMenuMessage(MMCODE_START, scr->MenuVerifyActiveWindow, &ie, IntuitionBase))
                    {
                        /* This lock will be released only when the user is
                           done with menus = when IECLASS_MENU + IESUBCLASS_MENUSTOP
                           event arrives (generated by MenuHandler task) */

                        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);
                        iihdata->MenuWindow = scr->MenuVerifyActiveWindow;
                        MENUS_ACTIVE = TRUE;
                    }

                    scr->MenuVerifyActiveWindow = NULL;
                    scr->MenuVerifyTimeOut = 0;
                    scr->MenuVerifyMsgCount = 0;
                    scr->MenuVerifySeconds = 0;
                    scr->MenuVerifyMicros = 0;
                    GetPrivIBase(IntuitionBase)->MenuVerifyScreen = NULL;
                }
            }
            else if (MENUS_ACTIVE)
            {
                FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
                keep_event = FALSE;
                break;
            }

#ifdef SKINS
            if (screen->MouseY <= screen->BarHeight && GetPrivScreen(screen)->SpecialFlags & SF_AppearingBar && !iihdata->TitlebarOnTop && iihdata->TitlebarAppearTime)
            {
                UQUAD currenttime = (((UQUAD)ie->ie_TimeStamp.tv_secs) * 50) + (UQUAD)(ie->ie_TimeStamp.tv_micro / 20000);
                if (currenttime >= iihdata->TitlebarAppearTime + 10)
                {
                    iihdata->TitlebarOnTop = TRUE;
                    iihdata->TitlebarAppearTime = 0;

                    LOCK_REFRESH(screen);

                    MoveLayer(0, screen->BarLayer, 0, screen->BarHeight + 1);
                    UpfrontLayer(0, screen->BarLayer);
                    CheckLayers(screen, IntuitionBase);

                    UNLOCK_REFRESH(screen);
                }
            }
#endif
            
            {
                UQUAD currenttime = (((UQUAD)ie->ie_TimeStamp.tv_secs) * 50) + (UQUAD)(ie->ie_TimeStamp.tv_micro / 20000);
                #define SECONDS(x) (x*50)
                if (iihdata->HelpGadgetFindTime && (currenttime >= iihdata->HelpGadgetFindTime + SECONDS(1)))
                {
            struct Gadget *lhg = iihdata->LastHelpGadget;
                    fire_intuimessage(iihdata->LastHelpWindow,
                                      IDCMP_GADGETHELP,
                                      lhg ? lhg->GadgetID : 0, /* Don't know what it should be */
                                      lhg,
                                      IntuitionBase);
                    iihdata->HelpGadgetFindTime = 0;
                }
            }
            
            if (gadget)
            {
                if (IS_BOOPSI_GADGET(gadget))
                {
                    gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
                }

            } /* if (gadget) */

#if USE_NEWDISPLAYBEEP

            if (GetPrivIBase(IntuitionBase)->BeepingScreens)
            {
                ULONG lock;
                struct Screen *scr;

                lock = LockIBase(0);

                for (scr = IntuitionBase->FirstScreen;
                     scr && GetPrivIBase(IntuitionBase)->BeepingScreens;
                     scr = scr->NextScreen)
                {
                    if ((scr->Flags & BEEPING) &&
                        !GetPrivScreen(scr)->BeepingCounter--)
                    {
                        GetPrivIBase(IntuitionBase)->BeepingScreens--;
                        scr->Flags &= (UWORD) ~BEEPING;

/*                      if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) <= 8)
                            // visual beep on CLUT-screen
                        {
                            // SetRGB4 (&screen->ViewPort, 0, scr->SaveColor0 & 0x000F, (scr->SaveColor0 & 0x00F0) >> 4, (scr->SaveColor0 & 0x0F00) >> 8);
                            SetRGB32 (&scr->ViewPort, 0,
                                      GetPrivScreen(scr)->DisplayBeepColor0[0],
                                      GetPrivScreen(scr)->DisplayBeepColor0[1],
                                      GetPrivScreen(scr)->DisplayBeepColor0[2]
                                     );
                        }
                        else
                            // visual beep on hi- and truecolor screens
                        {
                            RenderScreenBar(scr, FALSE, IntuitionBase);
                        }*/

                        RenderScreenBar(scr, FALSE, IntuitionBase);
                    }
                }

                UnlockIBase(lock);
            }
#endif /* USE_NEWDISPLAYBEEP */

            if (!w) break;

            /* Send INTUITICK msg only if app already replied the last INTUITICK msg */
            if (w->Flags & WFLG_WINDOWTICKED) break;

            if (w->IDCMPFlags & IDCMP_INTUITICKS)
            {
                /* Set the WINDOWTICKED flag, it will be cleared again when the app
                   replies back the msg and the InputHandler handles the replymsg
                   in HandleIntuiReplyPort() */

                ih_fire_intuimessage(w,
                             IDCMP_INTUITICKS,
                             0,
                             w,
                             IntuitionBase);
            }
            break; /* case IECLASS_TIMER */

        case IECLASS_MENU:
            if (MENUS_ACTIVE && (ie->ie_SubClass == IESUBCLASS_MENUSTOP))
            {
                struct Window *eventwin = (struct Window *)ie->ie_EventAddress;

                iihdata->MenuWindow = NULL;
                MENUS_ACTIVE = FALSE;

                /* semaphore was locked when menu action started, see
                   above where MMCODE_START MenuMessage is sent.

                   It could have also have been looked if the user
                   activated one of the menu key shortcuts, see
                   "case IECLASS_RAWKEY" */

                ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MenuLock);

                keep_event = FALSE;

                if (((struct IntWindow *)eventwin)->menulendwindow)
                {
                    eventwin = ((struct IntWindow *)eventwin)->menulendwindow;
                }

                ih_fire_intuimessage((struct Window *)eventwin,
                                 IDCMP_MENUPICK,
                                 ie->ie_Code,
                                 (struct Window *)ie->ie_EventAddress,
                                 IntuitionBase);

            }
            break;

        case IECLASS_DISKINSERTED:
        case IECLASS_DISKREMOVED:
        case IECLASS_NEWPREFS:
            {
                struct Screen *scr;
                ULONG            idcmp;
                LONG            lock;

                switch (ie->ie_Class)
                {
                case IECLASS_DISKINSERTED:
                    idcmp = IDCMP_DISKINSERTED;
                    break;
            
                case IECLASS_DISKREMOVED:
                    idcmp = IDCMP_DISKREMOVED;
                    break;
            
                default:
                    idcmp = IDCMP_NEWPREFS;
                    /*
                     * Here we need to update the mouse prefs and
                     * maybe other stuff which comes from the global prefs file.
                     */
                    break;
                }

                lock = LockIBase(0);

                for (scr = IntuitionBase->FirstScreen; scr; scr = scr->NextScreen)
                {
                    struct Window *win;
            
                    for (win = scr->FirstWindow; win; win = win->NextWindow)
                    {
                /* stegerg:
               CHECKME, really use fire_intuimessage() here,
               instead of ih_fireintuimessage? Same for
               IDCMP_GADGETHELP above, BTW. */
               
                        fire_intuimessage(win,
                                  idcmp,
                                  0,
                                  NULL,
                                  IntuitionBase);
                    }
                }

                UnlockIBase(lock);
            }
            break;

        case IECLASS_NEWMOUSE:
#ifdef __MORPHOS__
            /*
             * The following is only needed on hardware not running
             * the NewMouse driver.
             */
            if (w->IDCMPFlags & IDCMP_RAWKEY && (!SysBase->MaxLocMem))
            {
                ih_fire_intuimessage(w,
                                 IDCMP_RAWKEY,
                                 ie->ie_Code,
                                 ie->ie_position.ie_addr, /* ie_dead.ie_prev[1|2]Down[Code|Qual]. 64 bit machine!? */
                                 IntuitionBase);
                keep_event = FALSE;
            }
            break;
#endif
        case IECLASS_NULL:
            break;

#ifdef __MORPHOS__
        case IECLASS_NEWTIMER:
            if (MENUS_ACTIVE)
            {
                FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
                keep_event = FALSE;
                break;
            }

            if (gadget)
            {
                if (gadget == iihdata->MasterSizeGadget)
                {
                    gadget = DoGPInput(gi, gadget, ie, GM_HANDLEINPUT, &reuse_event, IntuitionBase);
                }

            } /* if (gadget) */
            break;
#endif /* __MORPHOS__ */

        default:
            if (MENUS_ACTIVE)
            {
                FireMenuMessage(MMCODE_EVENT, 0, ie, IntuitionBase);
                keep_event = FALSE;
                break;
            }
            
            bug
            (
                "[Intui] InputHandler: Unknown IEClass: addr = %x  class = %d (origclass = %d)\n",
                orig_ie, ie->ie_Class,orig_ie->ie_Class
            );
            
            break;
        } /* switch (ie->ie_Class) */

        if (reuse_event)
        {
        }
        else if (keep_event && !ie_used)
        {
            *iihdata->EndInputEventChain = orig_ie;
            iihdata->EndInputEventChain = &orig_ie->ie_NextEvent;
            ie_used = TRUE;
        }
        else if (!ie_used)
        {
            orig_ie->ie_NextEvent = iihdata->FreeInputEvents;
            iihdata->FreeInputEvents = orig_ie;
        }

    } /* for (each event in the chain) */

    iihdata->ActiveGadget = gadget;

    D(bug("Outside pollingloop\n"));

#if SINGLE_SETPOINTERPOS_PER_EVENTLOOP
    if (call_setpointerpos)
        MySetPointerPos(IntuitionBase);
#endif

    /* Terminate the event chain. */
    *iihdata->EndInputEventChain = NULL;

    /* Transfer the list of allocated events in the list of events that should
     * be freed the next time the handler is entered.
     */
    iihdata->AllocatedInputEventList = iihdata->NewAllocatedInputEventList;
    NEWLIST((struct List*)&iihdata->NewAllocatedInputEventList);

    /* Reset the event chain here, not at the beginning of the handler, for
     * events that might be allocated in other handers.
     */
    iihdata->EndInputEventChain = &iihdata->ReturnInputEvent;
    iihdata->FreeInputEvents = NULL;

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);

    //    DEBUG_HANDLER(dprintf("Handler: ->IBase 0x%lx KeyMapBase 0x%lx\n",IntuitionBase,KeymapBase));

    return iihdata->ReturnInputEvent;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

