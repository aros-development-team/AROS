/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <intuition/pointerclass.h>
#include <prefs/pointer.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "intuition_intern.h"
#include "inputhandler.h"
#include "inputhandler_support.h"
#include "monitorclass_private.h"

void MySetPointerPos(struct IntuitionBase *IntuitionBase)
{
    Object *mon = GetPrivIBase(IntuitionBase)->ActiveMonitor;

    if (mon)
	DoMethod(mon, MM_SetPointerPos, IntuitionBase->MouseX, IntuitionBase->MouseY);
}

BOOL ResetPointer(struct IntuitionBase *IntuitionBase)
{

    Object *mon;
    struct SharedPointer *pointer = NULL;
    Object *obj = GetPrivIBase(IntuitionBase)->DefaultPointer;
    BOOL res = TRUE;

    if (obj)
	GetAttr(POINTERA_SharedPointer, obj, (IPTR *)&pointer);
    D(bug("[ResetPointer] Default pointer is 0x%p\n", pointer));
    if (!pointer)
	return TRUE;

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    ForeachNode(&GetPrivIBase(IntuitionBase)->MonitorList, mon) {
	if (!FindFirstScreen(mon, IntuitionBase)) {
	    D(bug("[ResetPointer] Setting default pointer for monitor 0x%p\n", mon));
	    if (!DoMethod(mon, MM_SetPointerShape, pointer))
		res = FALSE;
	}
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    D(bug("[ResetPointer] Returning %d\n", res));
    return res;
}

void ActivateMonitor(Object *newmonitor, WORD x, WORD y, struct IntuitionBase *IntuitionBase)
{
    Object *oldmonitor = GetPrivIBase(IntuitionBase)->ActiveMonitor;

    D(bug("ActivateMonitor(0x%p), old monitor 0x%p\n", newmonitor, oldmonitor));
    /* Do not bother if switching to the same monitor */
    if (newmonitor == oldmonitor)
	return;

    if (oldmonitor)
	SetAttrs(oldmonitor, MA_PointerVisible, FALSE, TAG_DONE);

    GetPrivIBase(IntuitionBase)->ActiveMonitor = newmonitor;
    if (newmonitor) {
	struct Screen *scr = FindFirstScreen(newmonitor, IntuitionBase);
	UWORD DWidth, DHeight;

	if (x == -1)
	    x = IntuitionBase->MouseX;
	if (y == -1)
	    y = IntuitionBase->MouseY;

	/* A crude copy from inputhandler.c. We should really handle this in monitorclass */
	if (scr)
	{
	    DWidth = scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MaxX - scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MinX;
	    DHeight = scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MaxY - scr->ViewPort.ColorMap->cm_vpe->DisplayClip.MinY;
	}
	else
	{
	    /* If there's no active screen, we take 160x160 as a limit */
	    DWidth = 159;
	    DHeight = 159;
	}
	if (x > DWidth)
	    x = DWidth;
	if (y > DHeight)
	    y = DHeight;

	D(bug("[ActivateMonitor] Mouse pointer coordinates: (%d, %d)\n", x, y));
	IntuitionBase->MouseX = x;
	IntuitionBase->MouseY = y;

	SetAttrs(newmonitor, MA_PointerVisible, TRUE, TAG_DONE);
	MySetPointerPos(IntuitionBase);
	notify_mousemove_screensandwindows(IntuitionBase);
    }
    D(bug("[ActivateMonitor] Done\n"));
}

struct Screen *FindFirstScreen(Object *monitor, struct IntuitionBase *IntuitionBase)
{
    struct Screen *scr;

    for (scr = IntuitionBase->FirstScreen; scr; scr = scr->NextScreen) {
	if (GetPrivScreen(scr)->IMonitorNode == monitor)
	    break;
    }
    return scr;
}

struct RastPort *MyCreateRastPort(struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct RastPort *newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);
    
    if (newrp)
    {
        InitRastPort(newrp);
    }

    return newrp;
}

struct RastPort *MyCloneRastPort(struct IntuitionBase *IntuitionBase, struct RastPort *rp)
{
    struct RastPort *newrp = NULL;

    if (rp)
    {
        newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);
        if (newrp)
        {
            CopyMem(rp, newrp, sizeof(struct RastPort));
        }
    }

    return newrp;
}

void MyFreeRastPort(struct IntuitionBase *IntuitionBase, struct RastPort *rp)
{
    if (rp->RP_Extra)
    {
    	/* Just in case... What if someone plays with ClipRects? */
    	FreeVec(rp->RP_Extra);
    }

    FreeMem(rp, sizeof(*rp));
}

#ifdef __MORPHOS__

BOOL IsLayerHiddenBySibling(struct Layer *layer, BOOL xx)
{
    struct Window *win = layer->Window;
    
    /* skip requesters attached to the same window. */
    while (layer->front && layer->front->Window == win)
    {
        layer = layer->front;
    }

    /* jDc: we need to care for layers that are on
    ** front of our layer, but don't cover it*/

    if (layer->front)
    {
        struct Layer *lay;
	
        for (lay = layer->front; lay; lay = lay->front)
        {
            struct Window *lwin = lay->Window;
	    
            if (lwin && win)
            {
                if (lwin->LeftEdge > win->LeftEdge + win->Width - 1) continue;
                if (lwin->LeftEdge + lwin->Width - 1 < win->LeftEdge) continue;
                if (lwin->TopEdge > win->TopEdge + win->Height - 1) continue;
                if (lwin->TopEdge + lwin->Height - 1 < win->TopEdge) continue;
                return TRUE;
            }
        }
        return NULL;
	
    } else return NULL;
}

#endif

struct TextFont *SafeReopenFont(struct IntuitionBase *IntuitionBase,
                                struct TextFont **fontptr)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct TextFont *ret = NULL, *font;

    /* Atomically lock the font before, so it can't go away
     */
    Forbid();

    font = *fontptr;
    if (font)
    {
        struct TextAttr ta;

        font->tf_Accessors++;
        Permit();

        /* Now really open it
         */
        ta.ta_Name  = font->tf_Message.mn_Node.ln_Name;
        ta.ta_YSize = font->tf_YSize;
        ta.ta_Style = font->tf_Style;
        ta.ta_Flags = font->tf_Flags;

        ret = OpenFont(&ta);

        /* Unlock it
         */
        Forbid();
        font->tf_Accessors--;
    }

    Permit();

    return ret;
}

Object *MakePointerFromData(struct IntuitionBase *IntuitionBase,
    const UWORD *source, int xOffset, int yOffset, int width, int height)
{
    struct TagItem pointertags[] = {
        {POINTERA_BitMap      , (IPTR)source},
        {POINTERA_XOffset     , xOffset      },
        {POINTERA_YOffset     , yOffset      },
	{SPRITEA_OldDataFormat, TRUE	     },
	{SPRITEA_Width	      , width	     },
	{SPRITEA_OutputHeight , height	     },
        {TAG_DONE                            }
    };

    return NewObjectA(GetPrivIBase(IntuitionBase)->pointerclass, NULL, pointertags);
}

Object *MakePointerFromPrefs(struct IntuitionBase *IntuitionBase, struct Preferences *prefs)
{
    SetPointerColors(IntuitionBase);
    
    return MakePointerFromData(IntuitionBase, prefs->PointerMatrix, prefs->XOffset, prefs->YOffset, 16, 16);
}

void InstallPointer(struct IntuitionBase *IntuitionBase, UWORD which, Object **old, Object *pointer)
{
    struct IntScreen 	*scr;
    struct Window   	*win;
    struct SharedPointer *oldpointer;
    struct SharedPointer *newpointer;
    Object *oldobject;

    ULONG lock = LockIBase(0);

    GetAttr(POINTERA_SharedPointer, *old, (IPTR *)&oldpointer);
    GetAttr(POINTERA_SharedPointer, pointer, (IPTR *)&newpointer);

    for (scr = GetPrivScreen(IntuitionBase->FirstScreen); scr; scr = GetPrivScreen(scr->Screen.NextScreen))
    {
        for (win = scr->Screen.FirstWindow; win; win = win->NextWindow)
        {
            if (((struct IntWindow *)win)->pointer == *old)
            {
                win->XOffset = newpointer->xoffset;
                win->YOffset = newpointer->yoffset;
            }
        }

        if (scr->Pointer == oldpointer)
        {
            DEBUG_POINTER(dprintf("InstallPointer: scr 0x%lx pointer 0x%lx sprite 0x%lx\n",
                                  scr, pointer, newpointer->sprite));
            if (DoMethod(scr->IMonitorNode, MM_SetPointerShape, newpointer))
            {
                ObtainSharedPointer(newpointer, IntuitionBase);
                ReleaseSharedPointer(oldpointer, IntuitionBase);
                scr->Pointer = newpointer;
            }
            else
            {
                DEBUG_POINTER(dprintf("InstallPointer: can't change pointer.\n"));
            }
        }
    }

    oldobject = *old;
    *old = pointer;
    /* Set new normal pointer image on all empty displays */
    if (which == WBP_NORMAL)
	ResetPointer(IntuitionBase);
    /* Dispose old pointer only after setting new one */
    DisposeObject(oldobject);

    UnlockIBase(lock);
}

void SetPointerColors(struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Color32 *p;
    int     	   k;
    ULONG   	   lock = LockIBase(0);
    /* Probably this should apply to Workbench screen and not to currently active one? */
    struct Screen *scr = IntuitionBase->ActiveScreen;

    DEBUG_POINTER(dprintf("SetPointerColors()\n");)

    p = GetPrivIBase(IntuitionBase)->Colors;

    if (scr)
    {
#ifndef ALWAYS_ALLOCATE_SPRITE_COLORS
        if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) < 9)
#endif
	{
	    UWORD firstcol = scr->ViewPort.ColorMap->SpriteBase_Even;
	    
	    /* Translate bank number and offset to color number - see graphics/getcolormap.c */
	    firstcol = (firstcol << 4) | (firstcol >> 8);
            for (k = 1; k < 4; ++k, ++p) {
	        DEBUG_POINTER(dprintf("Color %u: R %08lx G %08lx B %08lx\n", p[k+7].red, p[k+7].green, p[k+7].blue);)
		SetRGB32(&scr->ViewPort, k + firstcol, p[k+7].red, p[k+7].green, p[k+7].blue);
	    }
        }
    }

    UnlockIBase(lock);

    DEBUG_POINTER(dprintf("SetPointerColors() done\n");)
}


struct SharedPointer *CreateSharedPointer(struct ExtSprite *sprite, int x, int y,
                    struct IntuitionBase *IntuitionBase)
{
    struct SharedPointer *pointer;

    pointer = AllocMem(sizeof(*pointer), MEMF_PUBLIC);
    if (pointer)
    {
        pointer->sprite = sprite;
        pointer->xoffset = x;
        pointer->yoffset = y;
        pointer->ref_count = 1;
    }

    return pointer;
}

void ObtainSharedPointer(struct SharedPointer *pointer,
                         struct IntuitionBase *IntuitionBase)
{
    ULONG lock = LockIBase(0);
    ++pointer->ref_count;
    UnlockIBase(lock);
}

void ReleaseSharedPointer(struct SharedPointer *pointer,
                          struct IntuitionBase *IntuitionBase)
{
    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    ULONG lock = LockIBase(0);
    if (--pointer->ref_count == 0)
    {
        FreeSpriteData(pointer->sprite);
        FreeMem(pointer, sizeof(*pointer));
    }
    UnlockIBase(lock);
}
