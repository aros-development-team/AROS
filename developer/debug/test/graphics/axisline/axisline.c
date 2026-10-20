/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <graphics/layers.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <string.h>

static ULONG failures;
static UBYTE expected[32][64];

static void reset(struct BitMap *bitmap)
{
    ULONG p, y, x;
    memset(expected, 6, sizeof(expected));
    for (p = 0; p < 4; p++)
        for (y = 0; y < 32; y++)
            for (x = 0; x < 8; x++)
                bitmap->Planes[p][y * bitmap->BytesPerRow + x] =
                    (6 & (1 << p)) ? 0xff : 0;
}

static void verify(struct BitMap *bitmap, CONST_STRPTR label)
{
    ULONG p, y, x, errors = 0;
    WaitBlit();
    for (y = 0; y < 32; y++)
        for (x = 0; x < 64; x++)
        {
            ULONG actual = 0;
            for (p = 0; p < 4; p++)
                actual |= ((bitmap->Planes[p][y * bitmap->BytesPerRow + x / 8]
                            >> (7 - (x & 7))) & 1) << p;
            if (actual != expected[y][x])
                errors++;
        }
    if (errors)
    {
        bug("AXIS LINE FAIL: %s: %lu pixels\n", label, (unsigned long)errors);
        failures++;
    }
}

static void exercise(ULONG flags)
{
    const WORD lines[][4] = {{3,4,50,4}, {50,6,3,6}, {10,2,10,27},
                             {20,27,20,2}, {30,10,30,10}};
    struct BitMap *bitmap = AllocBitMap(64, 32, 4, BMF_CLEAR | flags, NULL);
    struct RastPort rp;
    ULONG i, variant;

    if (!bitmap)
    {
        failures++;
        return;
    }
    InitRastPort(&rp);
    rp.BitMap = bitmap;
    SetAPen(&rp, 9);
    SetBPen(&rp, 3);
    for (variant = 0; variant < 4; variant++)
        for (i = 0; i < 5; i++)
        {
            WORD x1 = lines[i][0], y1 = lines[i][1];
            WORD x2 = lines[i][2], y2 = lines[i][3];
            WORD dx = x2 > x1 ? 1 : x2 < x1 ? -1 : 0;
            WORD dy = y2 > y1 ? 1 : y2 < y1 ? -1 : 0;
            WORD length = dx ? (x2 - x1) * dx : (y2 - y1) * dy;
            WORD n;
            UBYTE mask = variant == 1 ? 5 : 15;
            UWORD pattern = variant == 3 ? 0xa55a : 0xffff;

            reset(bitmap);
            rp.Mask = mask;
            SetDrMd(&rp, variant == 2 ? COMPLEMENT : JAM2);
            SetDrPt(&rp, pattern);
            Move(&rp, x1, y1);
            Draw(&rp, x2, y2);
            for (n = 0; n <= length; n++)
            {
                WORD x = x1 + n * dx, y = y1 + n * dy;
                UBYTE pen = pattern & (1 << (15 - (n & 15))) ? 9 : 3;
                expected[y][x] = variant == 2 ? 6 ^ mask :
                    (6 & ~mask) | (pen & mask);
            }
            verify(bitmap, "direction, pattern, draw mode and plane mask");
            if (rp.cp_x != x2 || rp.cp_y != y2)
                failures++;
        }

    reset(bitmap);
    rp.Mask = 15;
    SetDrMd(&rp, COMPLEMENT);
    SetDrPt(&rp, 0xffff);
    Move(&rp, 3, 16);
    Draw(&rp, 30, 16);
    Draw(&rp, 30, 28);
    for (i = 3; i <= 30; i++)
        expected[16][i] = 6 ^ 15;
    for (i = 16; i <= 28; i++)
        expected[i][30] = 6 ^ 15;
    verify(bitmap, "connected complement lines paint shared vertex once");

    {
        struct Layer_Info *info = NewLayerInfo();
        struct Layer *layer = info ? CreateUpfrontLayer(info, bitmap,
            8, 4, 40, 24, LAYERSIMPLE, NULL) : NULL;
        if (layer)
        {
            reset(bitmap);
            SetAPen(layer->rp, 9);
            Move(layer->rp, -5, 7);
            Draw(layer->rp, 50, 7);
            for (i = 8; i <= 40; i++)
                expected[11][i] = 9;
            verify(bitmap, "layer clips translated horizontal line");
            DeleteLayer(0, layer);
        }
        else
            failures++;
        if (info)
            DisposeLayerInfo(info);
    }
    FreeBitMap(bitmap);
}

int main(void)
{
    exercise(0);
    exercise(BMF_INTERLEAVED);
    bug("AXIS LINE TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
