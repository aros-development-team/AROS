/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <proto/oop.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <hidd/amigavideo.h>

BOOL internal_TryRawBlit(struct gfxdisplay_data *driver,
    struct BitMap *src, struct BitMap *dst, WORD sx, WORD sy, WORD dx, WORD dy,
    WORD width, WORD height, ULONG minterm, UBYTE mask, struct GfxBase *GfxBase)
{
    OOP_MethodID HiddAmigaGfxBase;

    if ((driver->display_flags & (DF_ExternalPlanar | DF_SoftCursor)) != DF_ExternalPlanar ||
        !src->Depth || src->Depth > 8 || src->Depth != dst->Depth)
        return FALSE;

    HiddAmigaGfxBase = OOP_GetMethodID(IID_Hidd_AmigaGfx, 0);
    return HIDD_AMIGAGFX_CopyBitMap(driver->display_gfxhidd, src, dst,
        sx, sy, dx, dy, width, height, MINTERM_TO_GCDRMD(minterm), mask);
}
