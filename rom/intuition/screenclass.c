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
#include "monitorclass_private.h"

/*i***************************************************************************************

    NAME
	--background_screenclass--

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

    if (mon)
    {
        ULONG lock = LockIBase(0);
        struct Screen *ret = FindFirstScreen(mon, IntuitionBase);

        if (ret && ((ret->Flags & SCREENTYPE) == CUSTOMSCREEN))
        {
            /* We want only public screens */
            ret = NULL;
        }

        UnlockIBase(lock);

        return (IPTR)ret;
    }

    return 0;
}

/*i***************************************************************************************

    NAME
        SA_Left

    SYNOPSIS
        [ISG], LONG

    LOCATION
        screenclass

    FUNCTION
        Position of left edge of the screen relative to top-left physical display corner.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_Top

    SYNOPSIS
	[ISG], LONG

    LOCATION
        screenclass

    FUNCTION
        Position of top edge of the screen relative to top-left physical display corner.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_Width

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Width of the screen (not display) in pixels.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_Height

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Height of the screen (not display) in pixels.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_Depth

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Depth of the screen.

    NOTES
        This attribute returns real depth, however old structure fields, for example
        screen->RastPort.BitMap->bm_Depth, in case of direct-color screens will
        contain 8 for purposes of backwards compatibility. 

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_PubName

    SYNOPSIS
	[I.G], STRPTR

    LOCATION
        screenclass

    FUNCTION
        Public screen name (system name, not human-readable title).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_DisplayID

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Display mode ID of the screen.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_Behind

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Supplying this attribute during screen creation (to OpenScreen() or
        OpenScreenTagList() functions) causes this screen to be created and
        opened behind all other screens.

        Querying this attribute returns its initial value.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_MonitorName

    SYNOPSIS
	[I.G], LONG

    LOCATION
        screenclass

    FUNCTION
        Name of the monitorclass object (AKA display device name) to which this screen
        belongs.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SA_MonitorObject

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_MonitorObject

    SYNOPSIS
	[..G], Object *

    LOCATION
        screenclass

    FUNCTION
        Returns display device (monitorclass) object to which this screen belongs

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SA_MonitorName

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_TopLeftScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in top-left diagonal
        direction relative to this screen's one. If the frontmost screen on the given
        monitor is not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_TopLeftMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_TopMiddleScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in top direction
        relative to this screen's one. If the frontmost screen on the given monitor is
        not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_TopMiddleMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_TopRightScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a screen displayed on a monitor placed in top-right diagonal
        direction relative to this screen's one. If the frontmost screen on the given
        monitor is not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_TopRightMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_MiddleLeftScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in left direction
        relative to this screen's one. If the frontmost screen on the given monitor is
        not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_MiddleLeftMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_MiddleRightScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in left direction
        relative to this screen's one. If the frontmost screen on the given monitor is
        not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_MiddleRightMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_BottomLeftScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in bottom-left
        diagonal direction relative to this screen's one. If the frontmost screen on
        the given monitor is not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_BottomLeftMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_BottomMiddleScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a public screen displayed on a monitor placed in bottom direction
        relative to this screen's one. If the frontmost screen on the given monitor is
        not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_BottomMiddleMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_BottomRightScreen

    SYNOPSIS
	[..G], struct Screen *

    LOCATION
        screenclass

    FUNCTION
	Get a pointer to a screen displayed on a monitor placed in bottom-right diagonal
        direction relative to this screen's one. If the frontmost screen on the given
        monitor is not public, NULL will be returned.

    NOTES
        AROS supports screen composition, so more than one screen is actually displayed
        at any given time. In order to match MorphOS semantics this attribute takes into
        account only the frontmost screen on the given monitor.

    EXAMPLE

    BUGS

    SEE ALSO
        monitorclass/MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_StopBlanker

    SYNOPSIS
	[S..], BOOL

    LOCATION
        screenclass

    FUNCTION
        This attribute is currently reserved and exists only for source compatibility with
        MorphOS.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_ShowPointer

    SYNOPSIS
	[ISG], BOOL

    LOCATION
        screenclass

    FUNCTION
        Setting this attribute to FALSE makes mouse pointer invisible on your custom screen.
        Default value is TRUE. Setting this attribute on public screens is ignore.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
/*i***************************************************************************************

    NAME
        SA_GammaControl

    SYNOPSIS
	[I..], BOOL

    LOCATION
        screenclass

    FUNCTION
        Setting this attribute to TRUE enables to use SA_GammaRed, SA_GammaBlue and
        SA_GammaGreen attributes to supply custom gamma correction table for your
        screen.

    NOTES
        Since in AROS more than one screen can be visible simultaneously, current
        display gamma correction table is determined by the frontmost screen on
        that display.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

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
        *msg->opg_Storage = screen->GammaControl.Active;
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
        *msg->opg_Storage = screen->Pens[BARDETAILPEN];
        break;

    case SA_ScreenbarTextFont:
        *msg->opg_Storage = (IPTR)screen->Screen.BarLayer->rp->Font;
        break;

    case SA_CompositingFlags:
        *msg->opg_Storage = (IPTR)(screen->SpecialFlags >> 8);
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

IPTR ScreenClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct IntScreen *screen = INST_DATA(cl, o);
    struct TagItem *tstate = msg->ops_AttrList;
    struct TagItem *tag;
    BOOL showpointer = screen->ShowPointer;
    BOOL movescreen = FALSE;
    BOOL gammaset = FALSE;

    while((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case SA_Left:
            if (screen->SpecialFlags & SF_Draggable)
            {
                screen->Screen.LeftEdge = tag->ti_Data;
                movescreen = TRUE;
            }
            break;

        case SA_Top:
            if (screen->SpecialFlags & SF_Draggable)
            {
                screen->Screen.TopEdge = tag->ti_Data;
                movescreen = TRUE;
            }
            break;

        case SA_ShowPointer:
            showpointer = tag->ti_Data;
            break;

        case SA_GammaRed:
            if (screen->GammaControl.UseGammaControl)
            {
                screen->GammaControl.GammaTableR = (UBYTE *)tag->ti_Data;
                gammaset = TRUE;
            }
            break;

        case SA_GammaBlue:
            if (screen->GammaControl.UseGammaControl)
            {
                screen->GammaControl.GammaTableB = (UBYTE *)tag->ti_Data;
                gammaset = TRUE;
            }
            break;

        case SA_GammaGreen:
            if (screen->GammaControl.UseGammaControl)
            {
                screen->GammaControl.GammaTableG = (UBYTE *)tag->ti_Data;
                gammaset = TRUE;
            }
            break;
        }
    }

    if (movescreen)
    {
        /*
         * Move the screen to (0, 0) relative of current position.
         * LeftEdge/TopEdge have been already updated.
         */
        ScreenPosition(&screen->Screen, SPOS_RELATIVE, 0, 0, 0, 0);
    }

    if (showpointer != screen->ShowPointer)
    {
        screen->ShowPointer = showpointer;
        /* TODO: Actually implement this */
    }

    if (gammaset)
    {
        /*
         * Update gamma table on the monitor.
         * The monitorclass takes care itself about whether this screen
         * currently controlls gamma table.
         */
        DoMethod((Object *)screen->IMonitorNode, MM_SetScreenGamma, &GetPrivScreen(screen)->GammaControl, FALSE);
    }

    return 0;
}
