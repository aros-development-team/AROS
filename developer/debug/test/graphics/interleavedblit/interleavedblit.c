/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

/* The native driver's public header must provide its own interface types. */
#include <hidd/amigavideo.h>
#include <aros/debug.h>
#include <devices/timer.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/timer.h>

struct Device *TimerBase;
static ULONG failures;
static UBYTE source_pixels[8][128], expected[8][128];

static UBYTE pixel(struct BitMap *bm, ULONG x, ULONG y)
{
    ULONG p;
    UBYTE value = 0;
    for (p = 0; p < bm->Depth; p++)
    {
        PLANEPTR plane = bm->Planes[p];
        if (plane == (PLANEPTR)-1)
            value |= 1 << p;
        else if (plane)
            value |= ((plane[y * bm->BytesPerRow + x / 8] >>
                       (7 - (x & 7))) & 1) << p;
    }
    return value;
}

static void fill(struct BitMap *bm, ULONG width, UBYTE seed)
{
    ULONG p, y, x;
    for (p = 0; p < 8; p++)
        for (y = 0; y < 8; y++)
            for (x = 0; x < width / 8; x++)
                bm->Planes[p][y * bm->BytesPerRow + x] =
                    seed + p * 31 + y * 17 + x * 7;
}

static void exercise(struct BitMap *src, struct BitMap *dst,
    WORD sx, WORD dx, WORD width, WORD height, UBYTE minterm, UBYTE mask)
{
    ULONG x, y, p, mismatches = 0;
    UBYTE write_mask = mask;
    if (src->Depth < dst->Depth)
        write_mask &= (1U << src->Depth) - 1;
    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 128; x++)
            source_pixels[y][x] = pixel(src, x, y);
        for (x = 0; x < 128; x++)
            expected[y][x] = pixel(dst, x, y);
    }
    for (y = 0; y < 8; y++)
        for (x = 0; x < 128; x++)
        {
            LONG source_x = (LONG)x + sx - dx;
            if (y >= height || (LONG)x < dx || (LONG)x >= dx + width ||
                source_x < 0 || source_x >= 128)
                continue;
            for (p = 0; p < 8; p++)
                if (write_mask & (1 << p))
                {
                    ULONG s = (source_pixels[y][x + sx - dx] >> p) & 1;
                    ULONG d = (expected[y][x] >> p) & 1;
                    ULONG value = (minterm >> (4 + 2 * s + d)) & 1;
                    expected[y][x] = (expected[y][x] & ~(1 << p)) | (value << p);
                }
        }
    BltBitMap(src, sx, 0, dst, dx, 0, width, height, minterm, mask, NULL);
    WaitBlit();
    for (y = 0; y < 8; y++)
        for (x = 0; x < 128; x++)
            if (pixel(dst, x, y) != expected[y][x])
                mismatches++;
    if (mismatches)
    {
        bug("INTERLEAVED BLIT FAIL: sx %d dx %d minterm %02x mask %02x: %lu pixels\n",
            sx, dx, minterm, mask, (unsigned long)mismatches);
        failures++;
    }
}

int main(void)
{
    struct BitMap *src = AllocBitMap(128, 8, 8, BMF_CLEAR | BMF_INTERLEAVED, NULL);
    struct BitMap *dst = AllocBitMap(128, 8, 8, BMF_CLEAR | BMF_INTERLEAVED, NULL);
    struct BitMap modified;
    struct MsgPort *port = NULL;
    struct timerequest *request = NULL;
    BOOL timer_open = FALSE;
    const UBYTE minterms[] = {0xc0, 0x80, 0xe0, 0x30, 0x60};
    ULONG i, shift, frequency;
    struct EClockVal start, end;

    if (!src || !dst)
    {
        failures++;
        goto out;
    }
    for (i = 0; i < 5; i++)
        for (shift = 0; shift < 3; shift++)
        {
            fill(src, 96, 13);
            fill(dst, 128, 87);
            exercise(src, dst, shift == 1 ? 7 : 2,
                     shift == 2 ? 7 : 2, 85, 5, minterms[i], 0xff);
        }
    fill(src, 96, 13);
    fill(dst, 128, 87);
    exercise(src, dst, 2, 7, 85, 5, 0xc0, 0x55);

    modified = *src;
    modified.Planes[1] = src->Planes[2];
    modified.Planes[2] = src->Planes[1];
    exercise(&modified, dst, 7, 2, 85, 5, 0xc0, 0xff);
    modified = *src;
    modified.Planes[1] = NULL;
    modified.Planes[2] = (PLANEPTR)-1;
    exercise(&modified, dst, 2, 7, 85, 5, 0x80, 0xff);
    modified = *dst;
    exercise(&modified, dst, 0, 0, 85, 5, 0x80, 0xff);
    modified = *src;
    modified.Planes[0] = NULL;
    exercise(&modified, dst, 2, 2, 85, 5, 0xc0, 0xff);
    modified.Planes[0] = (PLANEPTR)-1;
    exercise(&modified, dst, 2, 2, 85, 5, 0xc0, 0xff);
    modified = *src;
    modified.Depth = 4;
    exercise(&modified, dst, 2, 2, 85, 5, 0xc0, 0xff);
    exercise(src, dst, -3, 2, 20, 5, 0xc0, 0xff);
    exercise(src, dst, 100, 120, 32, 5, 0xc0, 0xff);

    port = CreateMsgPort();
    if (port)
        request = (struct timerequest *)CreateIORequest(port, sizeof(*request));
    if (request && OpenDevice(TIMERNAME, UNIT_ECLOCK,
                             (struct IORequest *)request, 0) == 0)
    {
        timer_open = TRUE;
        TimerBase = request->tr_node.io_Device;
        frequency = ReadEClock(&start);
        for (i = 0; i < 512; i++)
            BltBitMap(src, 0, 0, dst, 0, 0, 16, 5, 0xc0, 0xff, NULL);
        WaitBlit();
        ReadEClock(&end);
        bug("INTERLEAVED BLIT TIME: %lu ticks / %lu Hz\n",
            (unsigned long)(end.ev_lo - start.ev_lo), (unsigned long)frequency);
        if (!frequency || end.ev_hi - start.ev_hi > 1 ||
            end.ev_lo - start.ev_lo >= frequency)
        {
            bug("INTERLEAVED BLIT FAIL: 512 small copies exceed one second\n");
            failures++;
        }
    }
    else
        failures++;
out:
    if (timer_open)
        CloseDevice((struct IORequest *)request);
    if (request)
        DeleteIORequest((struct IORequest *)request);
    if (port)
        DeleteMsgPort(port);
    if (dst)
        FreeBitMap(dst);
    if (src)
        FreeBitMap(src);
    bug("INTERLEAVED BLIT TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
