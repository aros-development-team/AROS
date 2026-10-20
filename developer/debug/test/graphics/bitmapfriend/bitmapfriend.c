/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <graphics/gfx.h>
#include <proto/graphics.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR label)
{
    if (!okay)
    {
        bug("BITMAP FRIEND FAIL: %s\n", label);
        failures++;
    }
}

static void exercise(ULONG flags)
{
    struct BitMap *friend = AllocBitMap(320, 8, 8, flags | BMF_CLEAR, NULL);
    struct BitMap *bitmap = NULL;
    struct BitMap *copy = NULL;
    ULONG x, y, p, stride, mismatches = 0;

    check(friend != NULL, "allocate friend");
    if (!friend)
        return;
    bitmap = AllocBitMap(320, 8, 8, BMF_CLEAR, friend);
    copy = AllocBitMap(320, 8, 8, BMF_CLEAR, NULL);
    check(bitmap && copy, "allocate image and copy");
    if (!bitmap || !copy)
        goto out;

    check(bitmap->BytesPerRow == friend->BytesPerRow,
          "image retains the friend row layout");
    check((bitmap->Flags & BMF_INTERLEAVED) == (friend->Flags & BMF_INTERLEAVED),
          "image retains interleaving");
    stride = friend->BytesPerRow;
    for (p = 0; p < 8; p++)
    {
        if (flags & BMF_INTERLEAVED)
            check(bitmap->Planes[p] == bitmap->Planes[0] + p * 40,
                  "image plane rows are adjacent");
        /* Load row data in the friend's format, as classic image loaders do.
         * Skip this on an incompatible allocation to avoid writing past it. */
        if (bitmap->BytesPerRow == stride)
            for (y = 0; y < 8; y++)
                for (x = 0; x < 40; x++)
                    bitmap->Planes[p][y * stride + x] = (p * 31 + y * 7 + x);
    }
    if (bitmap->BytesPerRow != stride)
        goto out;

    BltBitMap(bitmap, 0, 0, copy, 0, 0, 320, 8, 0xc0, 0xff, NULL);
    WaitBlit();
    for (p = 0; p < 8; p++)
        for (y = 0; y < 8; y++)
            for (x = 0; x < 40; x++)
                if (copy->Planes[p][y * copy->BytesPerRow + x] !=
                    (UBYTE)(p * 31 + y * 7 + x))
                    mismatches++;
    check(mismatches == 0, "loaded image copies with every pixel intact");
out:
    if (copy)
        FreeBitMap(copy);
    if (bitmap)
        FreeBitMap(bitmap);
    FreeBitMap(friend);
}

int main(void)
{
    exercise(BMF_DISPLAYABLE | BMF_INTERLEAVED);
    exercise(BMF_DISPLAYABLE);
    bug("BITMAP FRIEND TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
