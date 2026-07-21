/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Amiga hardware implementation of graphics.library/BltClear().
*/

#include <hardware/custom.h>
#include <proto/graphics.h>

#include "graphics_intern.h"

/* See rom/graphics/bltclear.c for documentation. */

AROS_LH3(void, BltClear,
    AROS_LHA(void *, memBlock, A1),
    AROS_LHA(ULONG, bytecount, D0),
    AROS_LHA(ULONG, flags, D1),
    struct GfxBase *, GfxBase, 50, Graphics)
{
    AROS_LIBFUNC_INIT

    volatile struct Custom *custom = (struct Custom *)0xdff000;
    UBYTE *dst = memBlock;
    ULONG words;

    if (flags & 2)
        bytecount = (bytecount & 0xffff) * (bytecount >> 16);

    words = bytecount >> 1;
    if (!words)
        return;

    OwnBlitter();

    while (words)
    {
        UWORD width;
        UWORD height;
        ULONG count;

        if (words >= 64)
        {
            width = 64;
            height = words / 64;
            if (height > 1024)
                height = 1024;
        }
        else
        {
            width = words;
            height = 1;
        }

        count = (ULONG)width * height;

        WaitBlit();
        custom->bltcon0 = 0x0100; /* D = 0, D channel enabled. */
        custom->bltcon1 = 0;
        custom->bltdmod = 0;
        custom->bltdpt = dst;
        custom->bltsize = (height << 6) | (width & 63);

        dst += count * 2;
        words -= count;
    }

    WaitBlit();
    DisownBlitter();

    AROS_LIBFUNC_EXIT
}
