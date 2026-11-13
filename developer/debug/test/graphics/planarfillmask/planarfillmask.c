/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>

static ULONG failures;
static void pixel(struct RastPort *rp, WORD x, WORD y, ULONG expected)
{
    ULONG actual = ReadPixel(rp, x, y);
    if (actual != expected) {
        bug("GRAPHICS GC FAIL: pixel %d,%d expected %lu got %lu\n", x, y,
            (unsigned long)expected, (unsigned long)actual);
        failures++;
    }
}

int main(void)
{
    struct BitMap *bm = AllocBitMap(32, 8, 4, BMF_CLEAR, NULL);
    struct RastPort rp;
    UWORD bits = 0xa000;

    if (!bm)
        return 20;
    InitRastPort(&rp);
    rp.BitMap = bm;
    SetAPen(&rp, 5);
    RectFill(&rp, 0, 0, 31, 7);
    pixel(&rp, 1, 1, 5);
    rp.Mask = 3;
    SetAPen(&rp, 10);
    RectFill(&rp, 0, 0, 3, 3);
    pixel(&rp, 1, 1, 6);
    SetDrMd(&rp, COMPLEMENT);
    RectFill(&rp, 0, 0, 3, 3);
    pixel(&rp, 1, 1, 5);

    rp.Mask = 0xff;
    SetAPen(&rp, 3);
    SetBPen(&rp, 8);
    SetDrMd(&rp, JAM2);
    BltTemplate((PLANEPTR)&bits, 0, 2, &rp, 4, 4, 3, 1);
    pixel(&rp, 4, 4, 3);
    pixel(&rp, 5, 4, 8);
    pixel(&rp, 6, 4, 3);
    SetDrMd(&rp, JAM1);
    BltTemplate((PLANEPTR)&bits, 0, 2, &rp, 8, 4, 3, 1);
    pixel(&rp, 8, 4, 3);
    pixel(&rp, 9, 4, 5);
    pixel(&rp, 10, 4, 3);

    WaitBlit();
    FreeBitMap(bm);
    bug("GRAPHICS GC TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
