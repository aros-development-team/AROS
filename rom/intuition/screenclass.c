/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <graphics/driver.h>
#include <graphics/sprite.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/monitorclass.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "intuition_intern.h"
#include "intuition_customize.h"

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	screenclass

    NOTES
	In AROS screens are BOOPSI objects. It is possible to modify certain properties
        of the screen by using SetAttrs() and GetAttr() functions.

	screenclass by itself is private to the system and does not have a public ID.
        The user can't create objects of this class manually. Screens are created and
        destroyed as usually, using OpenScreen() and CloseScreen() functions.

	This class is fully compatible with MorphOS starting from v2.x.

*****************************************************************************************/

IPTR ScreenClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct IntScreen *screen = INST_DATA(cl, o);

    /* Free decoration objects */
    DisposeObject(screen->WinDecorObj);
    DisposeObject(screen->MenuDecorObj);
    DisposeObject(screen->ScrDecorObj);

    /* Free decoration buffer */
    FreeMem((APTR)screen->DecorUserBuffer, screen->DecorUserBufferSize);

    if ((screen->Decorator != ((struct IntIntuitionBase *)(IntuitionBase))->Decorator) &&
        (screen->Decorator != NULL))
    {
        struct NewDecorator *nd = screen->Decorator;

        ObtainSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);

        nd->nd_cnt--;
        if (nd->nd_IntPattern == NULL)
        {
            /*
             * If no pattern is defined, we assume it was old default decorator
             * which fell out of use. Unload it.
             */
            int_UnloadDecorator(nd, IntuitionBase);
        }

        ReleaseSemaphore(&((struct IntIntuitionBase *)(IntuitionBase))->ScrDecorSem);
    }

    return DoSuperMethodA(cl, o, msg);
}

static IPTR GetScreen(ULONG attrID, struct IntScreen *screen, struct IntuitionBase *IntuitionBase)
{
    Object *mon;

    GetAttr(attrID, screen->IMonitorNode, (IPTR *)&mon);
    return mon ? (IPTR)FindFirstScreen(mon, IntuitionBase) : 0;
}

IPTR ScreenClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddBitMapAttrBase = GetPrivIBase(IntuitionBase)->HiddAttrBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct IntScreen *screen = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
    case SA_Left:
        *msg->opg_Storage = screen->Screen.LeftEdge;
        break;

    case SA_Top:
        *msg->opg_Storage = screen->Screen.TopEdge;
        break;

    case SA_Width:
        *msg->opg_Storage = screen->Screen.Width;
        break;

    case SA_Height:
        *msg->opg_Storage = screen->Screen.Height;
        break;

    case SA_Depth:
        *msg->opg_Storage = screen->realdepth;
        break;

    case SA_PubName:
        *msg->opg_Storage = (IPTR)screen->pubScrNode->psn_Node.ln_Name;
        break;

    case SA_DisplayID:
        *msg->opg_Storage = screen->ModeID;
        break;

    case SA_Behind:
        *msg->opg_Storage = (screen->Screen.Flags & SCREENBEHIND) ? TRUE : FALSE;
        break;

    case SA_Displayed:
        *msg->opg_Storage = (FindFirstScreen(screen->IMonitorNode, IntuitionBase)
                             == &screen->Screen) ? TRUE : FALSE;
        break;

    case SA_MonitorName:
        return GetAttr(MA_MonitorName, screen->IMonitorNode, msg->opg_Storage);

    case SA_MonitorObject:
        *msg->opg_Storage = (IPTR)screen->IMonitorNode;
        break;

    case SA_TopLeftScreen:
        *msg->opg_Storage = GetScreen(MA_TopLeftMonitor, screen, IntuitionBase);
        break;

    case SA_TopMiddleScreen:
        *msg->opg_Storage = GetScreen(MA_TopMiddleMonitor, screen, IntuitionBase);
        break;

    case SA_TopRightScreen:
        *msg->opg_Storage = GetScreen(MA_TopRightMonitor, screen, IntuitionBase);
        break;

    case SA_MiddleLeftScreen:
        *msg->opg_Storage = GetScreen(MA_MiddleLeftMonitor, screen, IntuitionBase);
        break;

    case SA_MiddleRightScreen:
        *msg->opg_Storage = GetScreen(MA_MiddleRightMonitor, screen, IntuitionBase);
        break;

    case SA_BottomLeftScreen:
        *msg->opg_Storage = GetScreen(MA_BottomLeftMonitor, screen, IntuitionBase);
        break;

    case SA_BottomMiddleScreen:
        *msg->opg_Storage = GetScreen(MA_BottomMiddleMonitor, screen, IntuitionBase);
        break;

    case SA_BottomRightScreen:
        *msg->opg_Storage = GetScreen(MA_BottomRightMonitor, screen, IntuitionBase);
        break;

    case SA_ShowPointer:
        *msg->opg_Storage = screen->ShowPointer;
        break;

    case SA_GammaRed:
        *msg->opg_Storage = (IPTR)screen->GammaControl.GammaTableR;
        break;

    case SA_GammaBlue:
        *msg->opg_Storage = (IPTR)screen->GammaControl.GammaTableB;
        break;

    case SA_GammaGreen:
        *msg->opg_Storage = (IPTR)screen->GammaControl.GammaTableG;
        break;

    case SA_DisplayWidth:
        *msg->opg_Storage = screen->Screen.ViewPort.DWidth;
        break;

    case SA_DisplayHeight:
        *msg->opg_Storage = screen->Screen.ViewPort.DHeight;
        break;

    case SA_PixelFormat:
        if (IS_HIDD_BM(screen->Screen.RastPort.BitMap))
        {
            OOP_Object *pixfmt;

            OOP_GetAttr(HIDD_BM_OBJ(screen->Screen.RastPort.BitMap), aHidd_BitMap_PixFmt, (IPTR *)&pixfmt);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_CgxPixFmt, msg->opg_Storage);
        }
        else
        {
            *msg->opg_Storage = -1;
        }
        break;

    case SA_ScreenbarTextYPos:
        *msg->opg_Storage = screen->Screen.BarVBorder + screen->Screen.BarLayer->rp->TxBaseline;
        break;

    case SA_ScreenbarTextPen:
        *msg->opg_Storage = BARDETAILPEN; /* CHECKME: Is this correct ? */
        break;

    case SA_ScreenbarTextFont:
        *msg->opg_Storage = (IPTR)screen->Screen.BarLayer->rp->Font;
        break;

    case SA_OpacitySupport:    /* These are reserved in AROS */
    case SA_SourceAlphaSupport:
    case SA_ScreenbarSignal:
    case SA_CompositingLayers:
        *msg->opg_Storage = 0;
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
