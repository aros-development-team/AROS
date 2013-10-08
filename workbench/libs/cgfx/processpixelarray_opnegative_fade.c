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

void ProcessPixelArrayNegativeFadeFunc(struct RastPort *opRast, struct Rectangle *opRect, ULONG tintval, struct Library *CyberGfxBase)
{
    D(bug("[Cgfx] %s(0x%08x)\n", __PRETTY_FUNCTION__, tintval));
    D(bug("[Cgfx] %s: NegativeFade operator not implemented\n", __PRETTY_FUNCTION__));
}
