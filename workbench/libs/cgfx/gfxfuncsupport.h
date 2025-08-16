/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

#include <hidd/gfx.h>
#include <cybergraphx/cybergraphics.h>

#include "cybergraphics_intern.h"

struct extcol_render_data
{
    struct BitMap      *destbm;
    HIDDT_Pixel         pixel;
    struct IntCGFXBase *CyberGfxBase;
};

UBYTE GetRectFmtBytesPerPixel(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase);
HIDDT_StdPixFmt GetHIDDRectFmt(UBYTE rectfmt, struct RastPort *rp,
    struct Library *CyberGfxBase);
