/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/
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

#include <proto/graphics.h>
#include <proto/arossupport.h>
#include <proto/alib.h>
#include <proto/oop.h>
#include "intuition_intern.h"
#undef GfxBase
#include "graphics_internal.h"
#define GfxBase _GfxBase

static struct IntuitionBase * IntuiBase;

static struct Library *OOPBase = NULL;


int intui_init (struct IntuitionBase * IntuitionBase)
{
    int t;
    


#warning FIXME: this is a hack
    IntuiBase = IntuitionBase;
    
    return TRUE;
}



int intui_open (struct IntuitionBase * IntuitionBase)
{
    
    /* Hack */
    if (!OOPBase)
    {
    	OOPBase = OpenLibrary("oop.library", 0);
    	if (!OOPBase)
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

    return FALSE;
}

void intui_CloseWindow (struct Window * w,
	    struct IntuitionBase * IntuitionBase)
{

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



