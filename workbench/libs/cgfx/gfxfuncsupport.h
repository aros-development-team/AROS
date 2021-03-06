/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include "cybergraphics_intern.h"

UBYTE GetRectFmtBytesPerPixel(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase);
HIDDT_StdPixFmt GetHIDDRectFmt(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase);
