/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/

#define AROS_USE_OOP

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <devices/keymap.h>
#include <devices/input.h>


#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/layers.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include "intuition_intern.h"

#undef GfxBase
#undef LayersBase

#include "graphics_internal.h"


#undef DEBUG
#undef SDEBUG
#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

static struct GfxBase *GfxBase = NULL;
static struct IntuitionBase * IntuiBase;
static struct Library *LayersBase = NULL;




int intui_init (struct IntuitionBase * IntuitionBase)
{
    


#warning FIXME: this is a hack
    IntuiBase = IntuitionBase;
    
    return TRUE;
}



int intui_open (struct IntuitionBase * IntuitionBase)
{
    
    /* Hack */
    if (!GfxBase)
    {
    	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    	if (!GfxBase)
    	    return FALSE;
    }	    
    
    if (!LayersBase)
    {
    	LayersBase = OpenLibrary("layers.library", 0);
    	if (!LayersBase)
    	    return FALSE;
    }	    


    return TRUE;
}

void intui_close (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_expunge (struct IntuitionBase * IntuitionBase)
{
    return;
}

void intui_SetWindowTitles (struct Window * win, UBYTE * text, UBYTE * screen)
{
}

int intui_GetWindowSize (void)
{
    return sizeof (struct Window);
}

int intui_OpenWindow (struct Window * w,
	struct IntuitionBase * IntuitionBase)
{
    /* Create a layer for the window */
    EnterFunc(bug("intui_OpenWindow(w=%p)\n", w));
    
    D(bug("screen: %p\n", w->WScreen));
    D(bug("bitmap: %p\n", w->WScreen->RastPort.BitMap));
    
    w->WLayer = CreateUpfrontLayer( &w->WScreen->LayerInfo
    		, w->WScreen->RastPort.BitMap
		, w->LeftEdge
		, w->TopEdge
		, w->LeftEdge + w->Width
		, w->TopEdge  + w->Height
		, 0
		, NULL);

     D(bug("Layer created: %p\n", w->WLayer));
    if (w->WLayer)
    {
    
        /* Window needs a rastport */
	w->RPort = w->WLayer->rp;
	
    	/* Create some gadgets for window */
	
	
	
	/* Refresh window frame */
	RefreshWindowFrame(w);
	
	ReturnBool("intui_OpenWindow", TRUE);
	
    }		
    
    ReturnBool("intui_OpenWindow", FALSE);
}

void intui_CloseWindow (struct Window * w,
	    struct IntuitionBase * IntuitionBase)
{
    DeleteLayer(0, w->WLayer);
    
}

void intui_RefreshWindowFrame(struct Window *w)
{
    /* Draw a frame around the window */
    struct RastPort *rp = w->RPort;
    
    EnterFunc(bug("intui_RefreshWindowFrame(w=%p)\n", w));
    
    SetAPen(rp, 1);
    D(bug("Pen set\n"));
    Move(rp, 0, 0);
    D(bug("RP moved set\n"));

    D(bug("Window dims: (%d, %d, %d, %d)\n"
    	, w->LeftEdge, w->TopEdge, w->Width, w->Height));
	
    Draw(rp, w->Width - 1, 0);
    D(bug("Line drawn\n"));
    
    Draw(rp, w->Width - 1, w->Height - 1);
    Draw(rp, 0,  w->Height - 1);
    Draw(rp, 0, 0);
    
    ReturnVoid("intui_RefreshWindowFrame");
}

void intui_WindowToFront (struct Window * window)
{

}

void intui_WindowToBack (struct Window * window)
{

}

void intui_MoveWindow (struct Window * window, WORD dx, WORD dy)
{

}

void intui_ChangeWindowBox (struct Window * window, WORD x, WORD y,
    WORD width, WORD height)
{

}


void intui_SizeWindow (struct Window * win, long dx, long dy)
{

}

void intui_WindowLimits (struct Window * win,
    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight)
{

}

void intui_ActivateWindow (struct Window * win)
{

}

LONG intui_RawKeyConvert (struct InputEvent * ie, STRPTR buf,
	LONG size, struct KeyMap * km)
{

    return 0;
} /* intui_RawKeyConvert */

void intui_BeginRefresh (struct Window * win,
	struct IntuitionBase * IntuitionBase)
{

} /* intui_BeginRefresh */

void intui_EndRefresh (struct Window * win, BOOL free,
	struct IntuitionBase * IntuitionBase)
{

} /* intui_EndRefresh */



