/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

void ProcessPixelArrayGradientFunc(struct RastPort *opRast, struct Rectangle *opRect, BOOL opHoriz, ULONG opOffset, ULONG opCol1, ULONG opCol2, BOOL opFull, struct Library *CyberGfxBase)
{
    bug ("[Cgfx] %s(0x%08x->0x%08x)\n", __PRETTY_FUNCTION__, opCol1, opCol2);
    bug ("[Cgfx] %s: Gradient operator not implemented\n", __PRETTY_FUNCTION__);
}
