/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/cybergraphics.h>

#include <hidd/graphics.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/types.h>

#include "cybergraphics_intern.h"

void ProcessPixelArrayAlphaFunc(struct RastPort *opRast, struct Rectangle *opRect, UBYTE alphalevel, struct Library *CyberGfxBase)
{
    bug ("[Cgfx] %s(%d)\n", __PRETTY_FUNCTION__, alphalevel);
    bug ("[Cgfx] %s: SetAlpha operator not implemented\n", __PRETTY_FUNCTION__);
}
