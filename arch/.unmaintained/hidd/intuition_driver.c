/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Driver for using gfxhidd for gfx output
    Lang: english
*/

#define AROS_USE_OOP
#define AROS_ALMOST_COMPATIBLE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <devices/keymap.h>
#include <devices/input.h>

#include <proto/exec.h>
#include <proto/layers.h>

#include <proto/graphics.h>
#include <proto/arossupport.h>
/* #include <proto/alib.h> */
#include "intuition_intern.h"
#include "inputhandler_support.h"
#include "gadgets.h"

#undef GfxBase
#undef LayersBase

#include "graphics_internal.h"

#include <proto/intuition.h>

#undef DEBUG
#undef SDEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

static struct GfxBase *GfxBase = NULL;
static struct IntuitionBase * IntuiBase;
static struct Library *LayersBase = NULL;

int intui_init (struct IntuitionBase * IntuitionBase)
{
    
    bug("**************************************************************\n"
        "* Someone called config/hidd/intuition_driver.c/intui_init() *\n"
	"* This is no longer necessary. Everything is now handled in  *\n"
	"* intuition.library!!! Fix this!                             *\n"
	"**************************************************************");

    return FALSE;

#if 0    
#warning FIXME: this is a hack
    IntuiBase = IntuitionBase;
    
    return TRUE;
#endif
}



int intui_open (struct IntuitionBase * IntuitionBase)
{
    bug("**************************************************************\n"
        "* Someone called config/hidd/intuition_driver.c/intui_open() *\n"
	"* This is no longer necessary. Everything is now handled in  *\n"
	"* intuition.library!!! Fix this!                             *\n"
	"**************************************************************");

    return FALSE;
    
#if 0    
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
#endif
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
    return sizeof (struct IntWindow);
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


