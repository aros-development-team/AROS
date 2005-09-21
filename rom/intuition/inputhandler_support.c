/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Support functions for InputHandler.
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/layers.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/keymap.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/interrupts.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <devices/inputevent.h>
#include <string.h>

#include "inputhandler.h"

#include "boopsigadgets.h"
#include "boolgadgets.h"
#include "propgadgets.h"
#include "strgadgets.h"
#include "gadgets.h"
#include "intuition_intern.h" /* EWFLG_xxx */
#include "inputhandler_actions.h"
#include "inputhandler_support.h"
#include "menus.h"

#ifdef SKINS
#   include "mosmisc.h"
#endif

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define DEBUG_WINDOWNEEDSREFRESH(x) ;
#define DEBUG_DOGPINPUT(x)      ;
#define DEBUG_HANDLECUSTOMRETVAL(x) ;
#define DEBUG_ACTIVATEGADGET(x)     ;
#define DEBUG_FIREINTUIMSG(x)       ;

#include <stddef.h>

/****************************************************************************************/

/*
  All screens and windows will be updated with the current position of
  the mouse pointer. The windows will receive relative mouse coordinates.
*/

/****************************************************************************************/

void notify_mousemove_screensandwindows(WORD x,
                                        WORD y,
                                        struct IntuitionBase * IntuitionBase)
{
    LONG    	   lock = LockIBase(0);
    struct Screen *scr = IntuitionBase->FirstScreen;

    while (NULL != scr)
    {
        struct Window * win = scr->FirstWindow;

        scr->MouseX = x;// - scr->LeftEdge;
        scr->MouseY = y;// - scr->TopEdge;

        /*
        ** Visit all windows of this screen
        */

        while (NULL != win)
        {
            UpdateMouseCoords(win);

            win = win -> NextWindow;
        }

        scr = scr->NextScreen;
    }

    UnlockIBase(lock);
}

/****************************************************************************************/

void send_intuimessage(struct IntuiMessage *imsg, struct Window *w,
                       struct IntuitionBase *IntuitionBase)
{
    SendIntuiMessage(w, imsg);
}

/****************************************************************************************/

void free_intuimessage(struct IntuiMessage *imsg,
                       struct IntuitionBase *IntuitionBase)
{
    FreeIntuiMessage(imsg);
}

/****************************************************************************************/

struct IntuiMessage *alloc_intuimessage(struct Window *w,
                                struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;

    imsg = AllocIntuiMessage(w);
    if (imsg)
    {
        if (w)
        {
            if (w->IDCMPFlags & IDCMP_DELTAMOVE)
            {
                struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

                imsg->MouseX = iihd->DeltaMouseX;
                imsg->MouseY = iihd->DeltaMouseY;
            }
            else
            {
                imsg->MouseX = w->MouseX;
                imsg->MouseY = w->MouseY;
            }
        }
        CurrentTime(&imsg->Seconds, &imsg->Micros);
    }

    return imsg;
}

/****************************************************************************************/

BOOL fire_intuimessage(struct Window *w,
                       ULONG Class,
                       UWORD Code,
                       APTR IAddress,
                       struct IntuitionBase *IntuitionBase)
{
    struct IntuiMessage *imsg;
    BOOL            	 result = FALSE;

    if ((w->IDCMPFlags & Class) && (w->UserPort))
    {
        if ((imsg = alloc_intuimessage(w, IntuitionBase)))
        {
            struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

            imsg->Class =    Class;
            imsg->Code      = Code;
            imsg->Qualifier = iihd->ActQualifier;
            if (Class == IDCMP_RAWKEY)
            {
                INT_INTUIMESSAGE(imsg)->prevCodeQuals = IAddress;
                imsg->IAddress = &INT_INTUIMESSAGE(imsg)->prevCodeQuals;
            }
            else
            {
                imsg->IAddress = IAddress;
            }

            send_intuimessage(imsg, w, IntuitionBase);

            result = TRUE;
        }
        else
        {
            DEBUG_FIREINTUIMSG(dprintf("fire_intuimessage: can't alloc imsg\n"));
        }
    }
    else
    {
        DEBUG_FIREINTUIMSG(dprintf("fire_intuimessage: no Userport or masked out idcmpflags\n"));
    }

    return result;
}

BOOL fire_message(struct Window *w,ULONG Class, UWORD Code, APTR IAddress, struct IntuitionBase *IntuitionBase)
{
    struct ExtIntuiMessage *imsg;
    BOOL            	    result = FALSE;

    if ((w->IDCMPFlags & Class) && (w->UserPort))
    {
        if ((imsg = (struct ExtIntuiMessage *)alloc_intuimessage(w, IntuitionBase)))
        {
            struct IIHData *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

            imsg->eim_IntuiMessage.Class     = Class;
            imsg->eim_IntuiMessage.Code      = Code;
            imsg->eim_IntuiMessage.Qualifier = iihdata->ActQualifier;
            if (Class == IDCMP_RAWKEY)
            {
                INT_INTUIMESSAGE(imsg)->prevCodeQuals = IAddress;
                imsg->eim_IntuiMessage.IAddress = &INT_INTUIMESSAGE(imsg)->prevCodeQuals;
            }
            else
            {
                imsg->eim_IntuiMessage.IAddress = IAddress;
            }

            if (iihdata->ActEventTablet && (w->MoreFlags & WMFLG_TABLETMESSAGES))
            {
                if ((imsg->eim_TabletData = AllocPooled(GetPrivIBase(IntuitionBase)->IDCMPPool,sizeof (struct TabletData))))
                {
                    memclr(imsg->eim_TabletData,sizeof (struct TabletData));
                    imsg->eim_TabletData->td_XFraction = iihdata->ActEventTablet->ient_ScaledXFraction;
                    imsg->eim_TabletData->td_YFraction = iihdata->ActEventTablet->ient_ScaledYFraction;
                    imsg->eim_TabletData->td_TabletX = iihdata->ActEventTablet->ient_TabletX;
                    imsg->eim_TabletData->td_TabletY = iihdata->ActEventTablet->ient_TabletY;
                    imsg->eim_TabletData->td_RangeX = iihdata->ActEventTablet->ient_RangeX;
                    imsg->eim_TabletData->td_RangeY = iihdata->ActEventTablet->ient_RangeY;
                    imsg->eim_TabletData->td_TagList = CloneTagItems(iihdata->ActEventTablet->ient_TagList);
                }
            }

            send_intuimessage(imsg, w, IntuitionBase);

            result = TRUE;
        }
        else
        {
            DEBUG_FIREINTUIMSG(dprintf("fire_intuimessage: can't alloc imsg\n"));
        }
    }
    else
    {
        DEBUG_FIREINTUIMSG(dprintf("fire_intuimessage: no Userport or masked out idcmpflags\n"));
    }

    return result;
}

/****************************************************************************************/

/*
   use ih_fire_intuimessage if A) the inputevent because of which
   you call this function might have to be eaten or modified
   by Intuition or B) an inputevent might have to be created
   by Intuition because of a deferred action.

   In any case this function may be called only from inside Intuition's
   InputHandler!!!!!!
*/

/****************************************************************************************/

BOOL ih_fire_intuimessage(struct Window * w,
                          ULONG Class,
                          UWORD Code,
                          APTR IAddress,
                          struct IntuitionBase *IntuitionBase)
{
    struct IIHData    *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct InputEvent *ie/* = iihd->ActInputEvent*/;

    BOOL result;

    DEBUG_FIREINTUIMSG(dprintf("ih_fire_intuimessage: win 0x%lx class 0x%lx code 0x%lx IAddress 0x%lx\n",
                	       w,
                	       Class,
                	       Code,
                	       IAddress));

    result = fire_message(w, Class, Code, IAddress, IntuitionBase);

    DEBUG_FIREINTUIMSG(dprintf("ih_fire_intuimessage: fire_intuimessage result 0x%lx\n",
                result));

    if (result /*&& ie*/)
    {
        /* was sent as IDCMP to window so eat inputevent */

        //ie->ie_Class = IECLASS_NULL;

    }

#if 0
    else if (ie/* && (ie->ie_Class != IECLASS_NULL) && !iihd->ActInputEventUsed*/)
    {

        /* ih_fire_intuimessage was called from inside Intuition's event handling loop */

        //iihd->ActInputEventUsed = TRUE;

        ie->ie_SubClass     = 0;
        ie->ie_Code 	    = Code;
      //ie->ie_Qualifier    = iihd->ActQualifier;
        ie->ie_EventAddress = IAddress;

        switch(Class)
        {
        case IDCMP_GADGETUP:
            /* Note: on the Amiga if a boopsi Gadget which is GA_Immediate
               and GA_RelVerify immediately in GM_GOACTIVE returns GMR_VERIFY,
               then this sends IDCMP_GADGETDOWN + IDCMP_GADGETUP. AROS does
               the same. But for changed inputevents (if window does not have this
               IDCMP Flags set) there will be only one IECLASS_GADGETDOWN
            */

            ie->ie_Class = IECLASS_GADGETUP;
            break;

        case IDCMP_GADGETDOWN:
            ie->ie_Class = IECLASS_GADGETDOWN;
            break;

        case IDCMP_ACTIVEWINDOW:
            ie->ie_Class = IECLASS_ACTIVEWINDOW;
            break;

        case IDCMP_INACTIVEWINDOW:
            ie->ie_Class = IECLASS_INACTIVEWINDOW;
            break;

        case IDCMP_CLOSEWINDOW:
            ie->ie_Class = IECLASS_CLOSEWINDOW;
            break;

        case IDCMP_MENUHELP:
            ie->ie_Class = IECLASS_MENUHELP;
            break;

        case IDCMP_MENUPICK:
            ie->ie_Class = IECLASS_MENULIST;
            break;

        case IDCMP_MOUSEBUTTONS:
        case IDCMP_MOUSEMOVE:
        case IDCMP_RAWKEY:
        case IDCMP_VANILLAKEY:
            break;

        default:
            D(bug("ih_fireintuimessage: unexpected IDCMP (%x) for an inputevent-handling-fireintuimessage!\n", Class));
            break;

        }
    }
#endif
    else //if (!ie)
    {
        /* ih_fire_intuimessage was called from inside Intuition's defered action handling routines */

        if ((ie = AllocInputEvent(iihd)))
        {
            switch(Class)
            {
            case IDCMP_NEWSIZE:
                ie->ie_Class = IECLASS_SIZEWINDOW;
                break;

            case IDCMP_CHANGEWINDOW:
                ie->ie_Class = IECLASS_CHANGEWINDOW;
                break;

            case IDCMP_ACTIVEWINDOW:
                ie->ie_Class = IECLASS_ACTIVEWINDOW;
                break;

            case IDCMP_INACTIVEWINDOW:
                ie->ie_Class = IECLASS_INACTIVEWINDOW;
                break;

            case IDCMP_CLOSEWINDOW:
                ie->ie_Class = IECLASS_CLOSEWINDOW;
                break;

            case IDCMP_GADGETUP:
                ie->ie_Class = IECLASS_GADGETUP;
                break;

            case IDCMP_GADGETDOWN:
                ie->ie_Class = IECLASS_GADGETDOWN;
                break;

            case IDCMP_REFRESHWINDOW:
                ie->ie_Class = IECLASS_REFRESHWINDOW;
                break;

            case IDCMP_MENUHELP:
                ie->ie_Class = IECLASS_MENUHELP;
                break;

            case IDCMP_MENUPICK:
                ie->ie_Class = IECLASS_MENULIST;
                break;

            default:
                D(bug("ih_fireintuimessage: unexpected IDCMP (0x%X) for a deferred-action-fireintuimessage!\n", Class));
                break;

            } /* switch(Class) */

            ie->ie_Code     	= Code;
            ie->ie_Qualifier    = iihd->ActQualifier;
            ie->ie_EventAddress = IAddress;
            CurrentTime(&ie->ie_TimeStamp.tv_secs, &ie->ie_TimeStamp.tv_micro);

            D(bug("ih_fireintuimessage: generated InputEvent. Class = 0x%X  Code = %d  EventAddress = 0x%X\n",
                  ie->ie_Class,
                  ie->ie_Code,
                  ie->ie_EventAddress));

        } /* if ((ie = AllocInputEvent(iihd))) */
    }

    DEBUG_FIREINTUIMSG(dprintf("ih_fire_intuimessage: result 0x%lx\n",
                result));

    return result;
}

/*********************************************************************/

/* This function must never be called with the layer/layerinfo locked,
 * otherwise a deadlock with ObtainGIRPort can happen.
 */
IPTR Locked_DoMethodA (struct Window *w, struct Gadget *g, Msg message, struct IntuitionBase *IntuitionBase)
{
    IPTR rc;
    BOOL lock = w && (g->GadgetType & GTYP_SYSGADGET &&
            ((g->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING ||
             (g->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SIZING));

    if (lock)
    {
        LOCK_REFRESH(w->WScreen);
    }
    
    LOCKGADGET
    rc = Custom_DoMethodA(g, message);   
    UNLOCKGADGET

    if (lock)
    {
        UNLOCK_REFRESH(w->WScreen);
    }

    return rc;
}

/*********************************************************************/

#undef Custom_DoMethodA
IPTR Custom_DoMethodA (struct IntuitionBase *IntuitionBase, struct Gadget *g, Msg message)
{
    if (g->MutualExclude)
    {
        return AROS_UFC4(IPTR, ((struct Hook *)g->MutualExclude)->h_Entry,
                 AROS_UFCA(struct Hook *, (struct Hook *)g->MutualExclude, A0),
                 AROS_UFCA(struct Gadget *, g, A2),
                 AROS_UFCA(APTR, message, A1),
                 AROS_UFCA(struct IntuitionBase *, IntuitionBase, A6));
    }
    else /* Not needed since gadgetclass sets MutualExclude, but doesn't hurt. */
        return DoMethodA((Object *)g, message);
}

/****************************************************************************************/

void NotifyDepthArrangement(struct Window *w, struct IntuitionBase *IntuitionBase)
{
    if(w->MoreFlags & WMFLG_NOTIFYDEPTH)
    {
        ih_fire_intuimessage(w,
                	     IDCMP_CHANGEWINDOW,
                	     CWCODE_DEPTH,
                	     0,
                	     IntuitionBase);
    }
}


/****************************************************************************************/

void PrepareGadgetInfo(struct GadgetInfo *gi, struct Screen *scr, struct Window *win,
                       struct Requester *req)
{
    gi->gi_Screen     	    = scr;
    gi->gi_Window     	    = win;
    gi->gi_Requester        = req;
    gi->gi_RastPort   	    = 0;
    gi->gi_Pens.DetailPen   = scr->DetailPen;
    gi->gi_Pens.BlockPen    = scr->BlockPen;
    gi->gi_DrInfo     	    = (APTR)&(((struct IntScreen *)gi->gi_Screen)->DInfo);
}

/****************************************************************************************/

void SetGadgetInfoGadget(struct GadgetInfo *gi, struct Gadget *gad,
                         struct IntuitionBase *IntuitionBase)
{
    struct IIHData *iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    SET_GI_RPORT(gi, gi->gi_Window, gi->gi_Requester, gad);
    InitRastPort(&iihd->GadgetInfoRastPort);

    iihd->GadgetInfoRastPort.Layer = gi->gi_RastPort->Layer;
    iihd->GadgetInfoRastPort.BitMap = gi->gi_RastPort->BitMap;

    SetFont(&iihd->GadgetInfoRastPort, gi->gi_DrInfo->dri_Font);

    gi->gi_Layer = gi->gi_RastPort->Layer;
    gi->gi_RastPort = &iihd->GadgetInfoRastPort;

    GetGadgetDomain(gad, gi->gi_Screen, gi->gi_Window, gi->gi_Requester, &gi->gi_Domain);
}

/****************************************************************************************/

void SetGPIMouseCoords(struct gpInput *gpi, struct Gadget *gad)
{
    struct GadgetInfo *gi = gpi->gpi_GInfo;

    WORD mousex, mousey;

    if (IS_SCREEN_GADGET(gad) || !gi->gi_Window)
    {
        mousex = gi->gi_Screen->MouseX;
        mousey = gi->gi_Screen->MouseY;
    }
    else
    {
        mousex = gi->gi_Window->MouseX;
        mousey = gi->gi_Window->MouseY;
    }

    gpi->gpi_Mouse.X = mousex - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, gi->gi_Requester);
    gpi->gpi_Mouse.Y = mousey - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, gi->gi_Requester);
}

/****************************************************************************************/

void HandleSysGadgetVerify(struct GadgetInfo *gi, struct Gadget *gadget,
                           struct IntuitionBase *IntuitionBase)
{
    switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
    {
    case GTYP_CLOSE:
#ifdef __MORPHOS__
        if (((struct IntWindow *)(gi->gi_Window))->specialflags & SPFLAG_IAMDEAD)
        {
            CrashedDispose(gi->gi_Window,IntuitionBase);
        }
	else
	{
#endif
            ih_fire_intuimessage(gi->gi_Window,
                         IDCMP_CLOSEWINDOW,
                         0,
                         gi->gi_Window,
                         IntuitionBase);
#ifdef __MORPHOS__
        }
#endif
        break;

    case GTYP_WDEPTH:
        if (FALSE == IsLayerHiddenBySibling(WLAYER(gi->gi_Window),FALSE))
        {
            /* Send window to back */
            WindowToBack(gi->gi_Window);
        }
        else
        {
            /* Send window to front */
            WindowToFront(gi->gi_Window);
        }
        break;

    case GTYP_WZOOM:
        ZipWindow(gi->gi_Window);
        break;

    case GTYP_SDEPTH:
        if (gi->gi_Screen == IntuitionBase->FirstScreen)
        {
            ScreenToBack(gi->gi_Screen);
        }
        else
        {
            ScreenToFront(gi->gi_Screen);
        }
        break;

    } /* switch(gad->GadgetType & GTYP_SYSTYPEMASK) */
}

/****************************************************************************************/

struct Gadget *HandleCustomGadgetRetVal(IPTR retval, struct GadgetInfo *gi, struct Gadget *gadget,
                                ULONG termination, BOOL *reuse_event,
                                struct IntuitionBase *IntuitionBase)
{
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

    DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: retval %ld gi 0x%lx gadget 0x%lx termination %ld reuse %ld\n",
                     retval,
                     gi,
                     gadget,
                     termination,
                     *reuse_event));

    if (retval != GMR_MEACTIVE)
    {
        struct gpGoInactive gpgi;

        DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: !GMR_MEACTIVE\n"));

        if (retval & GMR_REUSE)
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: GMR_REUSE\n"));
            *reuse_event = TRUE;
        }

        if (retval & GMR_VERIFY)
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: GMR_VERIFY\n"));
            if (IS_SYS_GADGET(gadget))
            {
                DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: SysGad\n"));
                HandleSysGadgetVerify(gi, gadget, IntuitionBase);
            }
            else
            {
                /* Not a system gadget. Send IDCMP_GADGETUP, but not
                   if it is a screen gadget where gi->gi_Window would
                   be NULL */

                DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: no sysgad\n"));
                if ((gadget->Activation & GACT_RELVERIFY) &&
                    (gi->gi_Window))
                {
                    DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: Send IDCMP_GADGETUP\n"));
                    ih_fire_intuimessage(gi->gi_Window,
                                 IDCMP_GADGETUP,
                                 termination & 0x0000FFFF,
                                 gadget,
                                 IntuitionBase);
                }

            } /* switch(gad->GadgetType & GTYP_SYSTYPEMASK) */

        } /* if (retval & GMR_VERIFY) */

        DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: Send GM_GOINACTIVE\n"));

        gpgi.MethodID   = GM_GOINACTIVE;
        gpgi.gpgi_GInfo = gi;
        gpgi.gpgi_Abort = 0;

        Locked_DoMethodA(gi->gi_Window, gadget, (Msg)&gpgi, IntuitionBase);

        if (SYSGADGET_ACTIVE)
        {
            /* Switch back from Master Drag or Size Gadget to
               real/original/app Size or Drag Gadget */

            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: SYSGADGET_ACTIVE\n"));
            gadget = iihdata->ActiveSysGadget;
            iihdata->ActiveSysGadget = NULL;

            if (IS_BOOPSI_GADGET(gadget))
            {
                Locked_DoMethodA(gi->gi_Window, gadget, (Msg)&gpgi, IntuitionBase);
            }

            #ifdef __MORPHOS__
            if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING2)
            {
                ih_fire_intuimessage(gi->gi_Window,
                                     IDCMP_GADGETUP,
                                     0,
                                     gadget,
                                     IntuitionBase);
            }
            #endif

            retval = 0;
        }

        if (retval & GMR_VERIFY && gi->gi_Requester && gadget->Activation & GACT_ENDGADGET)
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: EndRequest\n"));
            EndRequest(gi->gi_Requester, gi->gi_Window);
            retval = 0;
        }

        gadget->Activation &= ~GACT_ACTIVEGADGET;

        DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: TabCycle 0x%lx retval 0x%lx\n",
                        		 (gadget->Flags & GFLG_TABCYCLE),
                        		 retval));

        if ((gadget->Flags & GFLG_TABCYCLE) && (retval & GMR_NEXTACTIVE))
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: TabCycle+GMR_NEXTACTIVE\n"));
            gadget = FindCycleGadget(gi->gi_Window, gi->gi_Requester, gadget, GMR_NEXTACTIVE);
        }
        else if ((gadget->Flags & GFLG_TABCYCLE) && (retval & GMR_PREVACTIVE))
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: TabCycle+GMR_PREVACTIVE\n"));
            gadget = FindCycleGadget(gi->gi_Window, gi->gi_Requester, gadget, GMR_PREVACTIVE);
        }
        else
        {
            gadget = NULL;
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: No gadget\n"));
        }

        if (gadget)
        {
            DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: activate gadget 0x%lx\n",gadget));
            gadget = DoActivateGadget(gi->gi_Window, gi->gi_Requester, gadget, IntuitionBase);
        }

    } /* if (retval != GMR_MEACTIVE) */
    else
    {
        DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: set GACT_ACTIVEGADGET\n"));
        gadget->Activation |= GACT_ACTIVEGADGET;
    }

    DEBUG_HANDLECUSTOMRETVAL(dprintf("HandleCustomGadgetRetVal: return 0x%x\n", gadget));
    return gadget;
}

/****************************************************************************************/

/* This function must never be called with the layer/layerinfo locked,
 * otherwise a deadlock with ObtainGIRPort can happen.
 */
struct Gadget *DoGPInput(struct GadgetInfo *gi, struct Gadget *gadget,
                         struct InputEvent *ie, STACKULONG methodid,
                         BOOL *reuse_event, struct IntuitionBase *IntuitionBase)
{
    struct IIHData  *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct gpInput   gpi;
    IPTR             retval;
    ULONG            termination;

    ie->ie_Qualifier = iihdata->ActQualifier;

    gpi.MethodID    	= methodid;
    gpi.gpi_GInfo   	= gi;
    gpi.gpi_IEvent  	= ie;
    gpi.gpi_Termination = &termination;
    gpi.gpi_TabletData  = NULL;

    SetGPIMouseCoords(&gpi, gadget);

    retval = Locked_DoMethodA (gi->gi_Window, gadget, (Msg)&gpi, IntuitionBase);

    DEBUG_DOGPINPUT(dprintf("DoGPInput: Locked_DoMethod gadget %p method 0x%lx retval %ld termination 0x%lx\n",
                gadget, methodid, retval, termination));

    return HandleCustomGadgetRetVal(retval, gi, gadget, termination,
                    reuse_event, IntuitionBase);

}

/****************************************************************************************/

struct Gadget * FindGadget (struct Screen *scr, struct Window * window,
                            struct Requester * req, int x, int y,
                            struct GadgetInfo * gi,BOOL sysonly,
                            struct IntuitionBase *IntuitionBase)
{
    struct Gadget   	*gadget, *firstgadget, *draggadget = 0;
    struct gpHitTest     gpht;
    struct IBox     	 ibox;
    WORD            	 xrel, yrel;
    BOOL            	 sys_only = sysonly;

    gpht.MethodID     = GM_HITTEST;
    gpht.gpht_GInfo   = gi;

    while (req || window || scr)
    {
        if (req)
        {
            firstgadget = req->ReqGadget;
        }
        else if (window)
        {
            firstgadget = window->FirstGadget;
        }
        else
        {
            if (draggadget) return draggadget;
            firstgadget = scr->FirstGadget;
        }

        for (gadget = firstgadget; gadget; gadget = gadget->NextGadget)
        {
            if (!(gadget->Flags & GFLG_DISABLED) &&
                (!sys_only ||
                 (gadget->GadgetType & GTYP_SYSGADGET &&
                   ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SIZING ||
                   (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING ||
                   (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDEPTH ||
                   (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_SDEPTH ||
                   (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WZOOM ||
                   (gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_CLOSE))))
            {
                /* stegerg: domain depends on gadgettype and windowflags! */
                GetGadgetDomain(gadget, scr, window, req, &gi->gi_Domain);

                /* Get coords relative to window */

                GetGadgetIBox(gadget, gi, &ibox);

                xrel = x - gi->gi_Domain.Left;
                yrel = y - gi->gi_Domain.Top;

                /*if (req)
            {
                    xrel -= req->LeftEdge + window->BorderLeft;
                    yrel -= req->TopEdge + window->BorderTop;
            }*/

                if (window)
                {
                    xrel -= window->LeftEdge;
                    yrel -= window->TopEdge;
                }

                if ((xrel >= ibox.Left) &&
                    (yrel >= ibox.Top) &&
                    (xrel < ibox.Left + ibox.Width) &&
                    (yrel < ibox.Top  + ibox.Height))
                {
                    if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == GTYP_WDRAGGING)
                    {
                        if (!draggadget) draggadget = gadget;
                    }
                    else
                    {

                        if ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
                        {

                            gpht.gpht_Mouse.X = xrel - ibox.Left;
                            gpht.gpht_Mouse.Y = yrel - ibox.Top;

                            /* jDc: don't check for == GMR_GADGETHIT since some reaction classes*/
                            /* (BURN IN HELL!) return TRUE here (related to imageclass HITEST?)*/
                            if (Locked_DoMethodA (window, gadget, (Msg)&gpht, IntuitionBase))
                                return (gadget);
                        }
                        else
                        {
                            return (gadget);
                        }
                    }
                }

            } /* if (!(gadget->Flags & GFLG_DISABLED)) */

        } /* for (gadget = window->FirstGadget; gadget; gadget = gadget->NextGadget) */

        sys_only = sysonly;

        if (req)
        {
            req = NULL;
            sys_only = TRUE;
        }
        else if (window)
        {
    	#ifdef SKINS
            draggadget = findbordergadget(window,draggadget,IntuitionBase);
    	#endif
            window = NULL;
        }
        else
            scr = NULL;
    }

    return (draggadget);

} /* FindGadget */


/****************************************************************************************/

struct Gadget * FindHelpGadget (struct Window * window,
                            int x, int y, struct IntuitionBase *IntuitionBase)
{
    struct Gadget   	*gadget, *firstgadget;
    struct Requester    *req = window->FirstRequest;

    while (req || window)
    {
        if (req)
        {
            firstgadget = req->ReqGadget;
        }
        else
        {
            firstgadget = window->FirstGadget;
        }

        for (gadget = firstgadget; gadget; gadget = gadget->NextGadget)
        {
            if ((gadget->Flags & GFLG_EXTENDED) &&
                (((struct ExtGadget *)gadget)->MoreFlags & GMORE_GADGETHELP))
            {
                if (InsideGadget(window->WScreen, window, req, gadget, x, y))
                {
                    return (gadget);
                }
            }

        }

        if (req)
            req = req->OlderRequest;
        else
            window = NULL;
    }

    return (NULL);

} /* FindGadget */


/****************************************************************************************/

BOOL InsideGadget(struct Screen *scr, struct Window *win, struct Requester *req,
                  struct Gadget *gad, WORD x, WORD y)
{
    struct IBox box;
    BOOL    	rc = FALSE;

    GetScrGadgetIBox(gad, scr, win, req, &box);

    if ((x >= box.Left) &&
        (y >= box.Top)  &&
        (x < box.Left + box.Width) &&
        (y < box.Top + box.Height))
    {
        rc = TRUE;
    }

    return rc;
}

/****************************************************************************************/

struct Gadget *DoActivateGadget(struct Window *win, struct Requester *req, struct Gadget *gad,
                            	struct IntuitionBase *IntuitionBase)
{
    struct IIHData  	*iihd = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct GadgetInfo   *gi = &iihd->GadgetInfo;
    struct Gadget   	*result = NULL;

    DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: Window 0x%lx Req 0x%lx Gadget 0x%lx\n",
                		 win,
                		 req,
                		 gad));

    DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: Activation 0x%lx\n",
                    	    	 gad->Activation));

    if (gad->Activation & GACT_IMMEDIATE)
    {
        DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: Send GADGETDOWN msg\n"));
        ih_fire_intuimessage(win,
                	     IDCMP_GADGETDOWN,
                	     0,
                	     gad,
                	     IntuitionBase);
    }

    PrepareGadgetInfo(gi, win->WScreen, win, req);
    SetGadgetInfoGadget(gi, gad, IntuitionBase);

    DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: Type 0x%lx\n",
                    gad->GadgetType & GTYP_GTYPEMASK));

    switch(gad->GadgetType & GTYP_GTYPEMASK)
    {
        case GTYP_STRGADGET:
            DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: GTYP_STRGADGET\n"));
            gad->Activation |= GACT_ACTIVEGADGET;
            UpdateStrGadget(gad, win, req, IntuitionBase);
            result = gad;
            break;

        case GTYP_CUSTOMGADGET:
        {
            struct gpInput  gpi;
            ULONG           termination;
            IPTR            retval;
            BOOL            reuse_event;

            DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: GTYP_CUSTOMGADGET\n"));
	    
            gpi.MethodID    	= GM_GOACTIVE;
            gpi.gpi_GInfo   	= gi;
            gpi.gpi_IEvent  	= NULL;
            gpi.gpi_Termination = &termination;
            gpi.gpi_Mouse.X 	= win->MouseX - gi->gi_Domain.Left - GetGadgetLeft(gad, gi->gi_Screen, gi->gi_Window, NULL);
            gpi.gpi_Mouse.Y 	= win->MouseY - gi->gi_Domain.Top  - GetGadgetTop(gad, gi->gi_Screen, gi->gi_Window, NULL);
            gpi.gpi_TabletData  = NULL;

            retval = Locked_DoMethodA (win, gad, (Msg)&gpi, IntuitionBase);

            gad = HandleCustomGadgetRetVal(retval, gi, gad,termination,
                               &reuse_event, IntuitionBase);

            if (gad)
            {
                gad->Activation |= GACT_ACTIVEGADGET;
                result = gad;
            }
            break;
        }

    } /* switch(gad->GadgetType & GTYP_GTYPEMASK) */

    DEBUG_ACTIVATEGADGET(dprintf("DoActivateGadget: result 0x%lx\n",
                    result));

    if (result) iihd->ActiveGadget = result;

    return result;
}


/****************************************************************************************/

struct Gadget *FindCycleGadget(struct Window *win, struct Requester *req,
                               struct Gadget *gad, WORD direction)
{
    struct Gadget *g = NULL, *gg, *prev, *first;

    D(bug("FindCycleGadget: win = %p  req %p gad = %p  direction = %d\n", win, req, gad, direction));

    if (req)
        first = req->ReqGadget;
    else
        first = win->FirstGadget;

    switch(direction)
    {
    case GMR_NEXTACTIVE:
        g = gad->NextGadget;
        if (!g) g = first;

        while(g)
        {
            if (g == gad)
            {
                if (!(gad->Flags & GFLG_TABCYCLE) || (gad->Flags & GFLG_DISABLED))
                {
                    /* should never happen */
                    g = NULL;
                }
                break;
            }
            if (!(g->Flags & GFLG_DISABLED) && (g->Flags & GFLG_TABCYCLE)) break;

            g = g->NextGadget;
            if (!g) g = first;
        }
        break;

    case GMR_PREVACTIVE:
        prev = 0;
        g = 0;
        gg = first;

        /* find a TABCYCLE gadget which is before gad in window's gadgetlist */
        while (gg)
        {
            if (gg == gad)
            {
                if (prev) g = prev;
                break;
            }
            if (!(gg->Flags & GFLG_DISABLED) && (gg->Flags & GFLG_TABCYCLE)) prev = gg;
            gg = gg->NextGadget;
        }

        if (gg && !g)
        {
            /* There was no TABCYCLE gadget before gad in window's gadgetlist */

            gg = gg->NextGadget;
            if (!gg)
            {
                if (!(gad->Flags & GFLG_DISABLED) && (gad->Flags & GFLG_TABCYCLE)) g = gad;
                break;
            }
            prev = 0;

            while(gg)
            {
                if (!(gg->Flags & GFLG_DISABLED) && (gg->Flags & GFLG_TABCYCLE)) prev = gg;
                gg = gg->NextGadget;
            }

            if (prev)
            {
                g = prev;
            }
            else
            {
                if (!(gad->Flags & GFLG_DISABLED) && (gad->Flags & GFLG_TABCYCLE)) g = gad;
            }
        }


        break;

    default: /* Unused, but well... */
        g = first;
        break;

    } /* switch(direction) */

    return g;
}

/****************************************************************************************/

void FixWindowCoords(struct Window *win, LONG *left, LONG *top, LONG *width, LONG *height,struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr = win->WScreen;

    if (*width < 1) *width = 1;
    if (*height < 1) *height = 1;

    if (*width > scr->Width) *width = scr->Width;
    if (*height > scr->Height) *height = scr->Height;

    if ((GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_OFFSCREENLAYERS) && (win->WScreen->LayerInfo.Flags & LIFLG_SUPPORTS_OFFSCREEN_LAYERS))
    {
        if (*left > scr->Width - 1) *left = scr->Width - 1;
        if (*top > scr->Height - 1) *top = scr->Height -1;
    }
    else
    {

        if ((*left + *width) > scr->Width)
        {
            *left = scr->Width - *width;
        }
        else if (*left < 0)
        {
            *left = 0;
        }

        if ((*top + *height) > scr->Height)
        {
            *top = scr->Height - *height;
        }
        else if (*top < 0)
        {
            *top = 0;
        }
    }
}

/****************************************************************************************/

void WindowNeedsRefresh(struct Window * w,
                        struct IntuitionBase * IntuitionBase )
{
    /* Supposed to send a message to this window, saying that it needs a
       refresh. I will check whether there is no such a message queued in
       its messageport, though. It only needs one such message!
    */

    /* Refresh the window's gadgetry ...
       ... stegerg: and in the actual implementation
       call RefershWindowFrame first, as the border gadgets don´t
       cover the whole border area.*/

    /*
        jDc: in actual implementation sizeevent means that we need to send
        idcmp, etc and do not clear the flag for smart_refresh window that
        has no idcmp_refreshwindow, otherwise we clear the flag!
    */

    DEBUG_WINDOWNEEDSREFRESH(dprintf("WindowNeedsRefresh: window 0x%lx gzz %d nocarerefresh %d\n",
                     w, IS_GZZWINDOW(w), IS_NOCAREREFRESH(w)));

    //trashregion means that we use delayed refreshing!

    if (
    #ifdef DAMAGECACHE
        (!IW(w)->trashregion) ||
    #else
        (!(w->Flags & WFLG_SIMPLE_REFRESH)) ||
    #endif
         IS_NOCAREREFRESH(w))
    {

        Gad_BeginUpdate(WLAYER(w), IntuitionBase);

        if (IS_NOCAREREFRESH(w) || (!((!(w->Flags & WFLG_SIMPLE_REFRESH)) && (!(IW(w)->specialflags & SPFLAG_LAYERRESIZED)))))
        {
            if (!IS_GZZWINDOW(w))
            {
                if (w->Flags & WFLG_BORDERLESS)
                {
                    int_refreshglist(w->FirstGadget, w, NULL, -1, 0, 0, IntuitionBase);
                }
		else
		{
                    int_refreshwindowframe(w,0,0,IntuitionBase);
                }
            }
            else
            {
                /* refresh all gadgets except border gadgets */
                int_refreshglist(w->FirstGadget, w, NULL, -1, 0, REFRESHGAD_BORDER, IntuitionBase);
            }
            IW(w)->specialflags &= ~SPFLAG_LAYERRESIZED;
        }

        if (IS_NOCAREREFRESH(w)) WLAYER(w)->Flags &= ~LAYERREFRESH;

        Gad_EndUpdate(WLAYER(w), IS_NOCAREREFRESH(w) ? TRUE : FALSE, IntuitionBase);

    } else {
    #ifdef DAMAGECACHE
        struct Rectangle rect;
        BOOL doclear = (w->Flags & WFLG_BORDERLESS) ? FALSE : TRUE;

        rect.MinX = w->BorderLeft;
        rect.MinY = w->BorderTop;
        rect.MaxX = w->Width - w->BorderRight - 1;
        rect.MaxY = w->Height - w->BorderBottom - 1;
    #endif

    #ifndef BEGINUPDATEGADGETREFRESH
        Gad_BeginUpdate(WLAYER(w), IntuitionBase);
    #else
    #ifdef DAMAGECACHE
        LockLayer(0,WLAYER(w));
    #endif
    #endif
    
    #ifndef BEGINUPDATEGADGETREFRESH
        if (!IS_GZZWINDOW(w))
        {
            if (w->Flags & WFLG_BORDERLESS)
            {
                int_refreshglist(w->FirstGadget, w, NULL, -1, 0, 0, IntuitionBase);
            }
	    else
	    {
                int_refreshwindowframe(w,0,0,IntuitionBase);
            }
        }
	else
	{
            /* refresh all gadgets except border and gadtools gadgets */
            int_refreshglist(w->FirstGadget, w, NULL, -1, 0, REFRESHGAD_BORDER , IntuitionBase);
        }
    #endif

    #ifdef DAMAGECACHE
        //add rects to trashregion here
        OrRegionRegion(WLAYER(w)->DamageList,IW(w)->trashregion);

        if (doclear)
        {
            ClearRectRegion(IW(w)->trashregion,&rect);
            AndRectRegion(WLAYER(w)->DamageList,&rect);
        }

        IW(w)->specialflags |= SPFLAG_LAYERREFRESH;
    #else
    #ifdef BEGINUPDATEGADGETREFRESH
        IW(w)->specialflags |= SPFLAG_LAYERREFRESH;
    #endif
    #endif

    #ifndef BEGINUPDATEGADGETREFRESH
        Gad_EndUpdate(WLAYER(w), FALSE, IntuitionBase);
    #else
    #ifdef DAMAGECACHE
        UnlockLayer(WLAYER(w));
    #endif
    #endif

    }

    if (IS_DOCAREREFRESH(w))
    {
        if (w->UserPort && (w->IDCMPFlags & IDCMP_REFRESHWINDOW))
        {
            struct IntuiMessage *IM;
            BOOL    	    	 found = FALSE;

            /* Can use Forbid() for this */
            Forbid();

            IM = (struct IntuiMessage *)w->UserPort->mp_MsgList.lh_Head;

            ForeachNode(&w->UserPort->mp_MsgList, IM)
            {
                /* Does the window already have such a message? */
                if (IDCMP_REFRESHWINDOW == IM->Class && IM->IAddress == w)
                {
                    DEBUG_WINDOWNEEDSREFRESH(dprintf("WindowNeedsRefresh: refresh pending\n"));
                    D(bug("Window %s already has a refresh message pending!!\n",
                          w->Title ? w->Title : (STRPTR)"<NONAME>"));
                    found = TRUE;
                    break;
                }
            }

            Permit();

            if (!found)
            {
                struct InputEvent *new_ie;
                struct IIHData    *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

                D(bug("Sending a refresh message to window %s  %d %d %d %d!!\n",
                      w->Title ? w->Title : (STRPTR)"<NONAME>",
                      w->LeftEdge,
                      w->TopEdge,
                      w->Width,
                      w->Height));

                DEBUG_WINDOWNEEDSREFRESH(dprintf("WindowNeedsRefresh: sending idcmp message\n"));

                if ((new_ie = AllocInputEvent(iihdata)))
                {
                    new_ie->ie_Class 	    = IECLASS_EVENT;
                    new_ie->ie_Code 	    = IECODE_REFRESH;
                    new_ie->ie_EventAddress = w;
                    CurrentTime(&new_ie->ie_TimeStamp.tv_secs, &new_ie->ie_TimeStamp.tv_micro);
                }

                fire_intuimessage(w,
                        	  IDCMP_REFRESHWINDOW,
                        	  0,
                        	  w,
                        	  IntuitionBase);
            } /* if (!found) */

        } /* if (w->UserPort && (w->IDCMPFlags & IDCMP_REFRESHWINDOW)) */
        else
        {
            struct IIHData *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;

            if (FindTask(NULL) == iihdata->InputDeviceTask)
            {
                struct InputEvent *new_ie;

                DEBUG_WINDOWNEEDSREFRESH(dprintf("WindowNeedsRefresh: sending inputevent\n"));

                if ((new_ie = AllocInputEvent(iihdata)))
                {
                    new_ie->ie_Class = IECLASS_EVENT;
                    new_ie->ie_Code = IECODE_REFRESH;
                    new_ie->ie_EventAddress = w;
                    CurrentTime(&new_ie->ie_TimeStamp.tv_secs, &new_ie->ie_TimeStamp.tv_micro);
                }

                ih_fire_intuimessage(w,
                             IDCMP_REFRESHWINDOW,
                             0,
                             w,
                             IntuitionBase);
            }
        }

    } /* if (!IS_NOCAREREFRESH(w)) */

}

/****************************************************************************************/

struct Window *FindActiveWindow(struct InputEvent *ie,ULONG *stitlebarhit,
                            struct IntuitionBase *IntuitionBase)
{
    /* The caller has checked that the input event is a IECLASS_RAWMOUSE, SELECTDOWN event */
    /* NOTE: may be called with NULL ie ptr! */
    struct Screen   *scr;
    struct Layer    *l;
    struct Window   *new_w;
    ULONG            lock;

    lock = LockIBase(0UL);

    new_w = IntuitionBase->ActiveWindow;
    scr   = IntuitionBase->FirstScreen;

    UnlockIBase(lock);

    D(bug("FindActiveWindow: scr %p win %p\n",scr,new_w));

    if (stitlebarhit) *stitlebarhit = FALSE;

    if (scr)
    {
        D(bug("FindActiveWindow: Click at (%d,%d)\n",scr->MouseX,scr->MouseY));

        /* What layer ? */
        LockLayerInfo(&scr->LayerInfo);

        l = WhichLayer(&scr->LayerInfo, scr->MouseX, scr->MouseY);

        UnlockLayerInfo(&scr->LayerInfo);

        if (NULL == l)
        {
            new_w = NULL;
            D(bug("FindActiveWindow: Click not inside layer\n"));
        }
        else if (l == scr->BarLayer)
        {
            D(bug("FindActiveWindow: Click on screen bar layer -> active window stays the same\n"));
            if (stitlebarhit) *stitlebarhit = TRUE;
        }
        else
        {
            new_w = (struct Window *)l->Window;
            if (!new_w)
            {
                D(bug("FindActiveWindow: Selected layer is not a window\n"));
            }

            D(bug("FindActiveWindow: Found layer %p\n", l));
        }
    }

    D(bug("FindActiveWindow: New window %p\n", new_w));
    return new_w;
}

/****************************************************************************************/

struct Window *FindDesktopWindow(struct Screen *screen,struct IntuitionBase *IntuitionBase)
{
    struct Window *win;
    
    for (win = screen->FirstWindow; win; win = win->NextWindow)
    {
        if (win->Flags & WFLG_BACKDROP &&
            win->Width == screen->Width &&
            win->Height >= screen->Height - (screen->BarHeight + 2))
        {
            return win;
        }
    }
    
    return NULL;
}


/****************************************************************************************/

struct InputEvent *AllocInputEvent(struct IIHData *iihdata)
{
    struct IntuitionBase    	*IntuitionBase = iihdata->IntuitionBase;
    struct GeneratedInputEvent  *gie;
    struct InputEvent       	*ie;

    /* There might be an inputevent from someone else that our handler discarded.
     * We may as well use it. This can only happen inside our main loop.
     */
    ie = iihdata->FreeInputEvents;
    if (ie)
    {
        iihdata->FreeInputEvents = ie->ie_NextEvent;
        DEBUG_INPUTEVENT(dprintf("AllocInputEvent: reuse 0x%lx event\n", ie));
    }
    else
    {
        gie = AllocPooled(iihdata->InputEventMemPool, sizeof(struct GeneratedInputEvent));
        if (gie)
        {
            /* Allocated events are put in the list of events that have not yet been
             * propagated.
             */
            AddTail((struct List *)&iihdata->NewAllocatedInputEventList, (struct Node *)gie);
            ie = &gie->ie;
        }
        DEBUG_INPUTEVENT(dprintf("AllocInputEvent: allocated 0x%lx (0x%lx)\n", ie, gie));
    }

    if (ie)
    {
        *iihdata->EndInputEventChain = ie;
        iihdata->EndInputEventChain = &ie->ie_NextEvent;
    }

    return ie;
}

/****************************************************************************************/

void FreeGeneratedInputEvents(struct IIHData *iihdata)
{
    struct IntuitionBase    *IntuitionBase = iihdata->IntuitionBase;
    struct Node             *node, *succ;

    /* Free the list of allocated events that have already been propagated. */
    ForeachNodeSafe(&iihdata->AllocatedInputEventList, node, succ)
    {
        DEBUG_INPUTEVENT(dprintf("FreeGeneratedInputEvent: free 0x%lx\n", node));
        FreePooled(iihdata->InputEventMemPool, node, sizeof(struct GeneratedInputEvent));
    }

    /* The list is not in a valid state at this point, and NewList() should
     * be called, but since we won't use it until the list of not-yet-propagated
     * events is copied in it, we won't bother.
     */
    //NEWLIST(&iihdata->AllocatedInputEventList);
}

/****************************************************************************************/

BOOL FireMenuMessage(WORD code, struct Window *win,
                     struct InputEvent *ie, struct IntuitionBase *IntuitionBase)
{
    struct MenuMessage *msg;
    BOOL              	result = FALSE;

    if ((msg = AllocMenuMessage(IntuitionBase)))
    {
        msg->code = code;
        msg->win  = win;
        if (ie) msg->ie = *ie;
        SendMenuMessage(msg, IntuitionBase);

        result = TRUE;
    }

    return result;
}

/****************************************************************************************/

LONG Gad_BeginUpdate(struct Layer *layer, struct IntuitionBase *IntuitionBase)
{
    /* Must lock GadgetLock to avoid deadlocks with ObtainGirPort
       from other tasks, because ObtainGirPort first obtains
       GadgetLock and then layer lock through LockLayer!!!! */
    LOCKGADGET
    return BeginUpdate(layer);
}

/****************************************************************************************/

void Gad_EndUpdate(struct Layer *layer, UWORD flag, struct IntuitionBase *IntuitionBase)
{
    EndUpdate(layer, flag);
    UNLOCKGADGET
}

/****************************************************************************************/
