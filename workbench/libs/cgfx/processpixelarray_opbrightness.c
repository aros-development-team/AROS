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

void ProcessPixelArrayBrightnessFunc(struct RastPort *opRast, struct Rectangle *opRect, LONG value, struct Library *CyberGfxBase)
{
    bug ("[Cgfx] %s(%d)\n", __PRETTY_FUNCTION__, value);
    bug ("[Cgfx] %s: Brightness operator not implemented\n", __PRETTY_FUNCTION__);
}
