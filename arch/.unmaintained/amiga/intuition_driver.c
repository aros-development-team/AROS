/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG_FreeMem 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/aros_protos.h>
#include "intuition_intern.h"
#include "gadgets.h"
#include "propgadgets.h"

#define DEBUG	0
#define DEBUG_ProcessXEvents	0

#if DEBUG
#   define D(x)     x
#else
#   define D(x)     /* eps */
#endif

#define bug	    kprintf

extern void _aros_not_implemented(void);

int intui_init (struct IntuitionBase * IntuitionBase)
{
    fprintf(stderr, "intuition driver init function goes here\n");
    return FALSE;
}

int intui_open (struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
    return FALSE;
}

void intui_close (struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
    return;
}

void intui_expunge (struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
    return;
}

void intui_SetWindowTitles (struct Window * win, UBYTE * text, UBYTE * screen)
{
    _aros_not_implemented();
}

int intui_GetWindowSize (void)
{
    _aros_not_implemented();
    return 0;
}

int intui_OpenWindow (struct Window * iw,
	struct IntuitionBase * IntuitionBase,
	struct BitMap        * SuperBitMap)
{
    _aros_not_implemented();
    return 0;
}

void intui_CloseWindow (struct Window * iw,
	    struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
}

void intui_WindowToFront (struct Window * window
                          struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
}

void intui_WindowToBack (struct Window * window,
                         struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
}

long StateToQualifier (unsigned long state)
{
    _aros_not_implemented();
    return 0;
} /* StateToQualifier */

void intui_SizeWindow (struct Window * win, long dx, long dy)
{
    _aros_not_implemented();
}

void intui_WindowLimits (struct Window * win,
    WORD MinWidth, WORD MinHeight, UWORD MaxWidth, UWORD MaxHeight)
{
    _aros_not_implemented();
}

void intui_ActivateWindow (struct Window * win)
{
    _aros_not_implemented();
}

LONG intui_RawKeyConvert (struct InputEvent * ie, STRPTR buf,
	LONG size, struct KeyMap * km)
{
    _aros_not_implemented();
    return 0;
} /* intui_RawKeyConvert */

void intui_BeginRefresh (struct Window * win,
	struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
} /* intui_BeginRefresh */

void intui_EndRefresh (struct Window * win, BOOL free, struct IntuitionBase * IntuitionBase)
{
    _aros_not_implemented();
} /* intui_EndRefresh */


struct Gadget * FindGadget (struct Window * window, int x, int y)
{
    _aros_not_implemented();
    return 0;
} /* FindGadget */

void intui_ProcessEvents (void)
{
    _aros_not_implemented();
}

