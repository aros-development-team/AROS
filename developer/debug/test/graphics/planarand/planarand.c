/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <devices/timer.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/timer.h>

struct Device *TimerBase;
static ULONG failures;

static void check(BOOL okay, CONST_STRPTR label)
{
    if (!okay)
    {
        bug("PLANAR AND FAIL: %s\n", label);
        failures++;
    }
}

static ULONG pattern(ULONG x, ULONG y, ULONG p, BOOL source)
{
    return source ? (((x ^ (y << 1) ^ p) >> 1) & 1) :
                    (((x + (y << 1) + p) >> 2) & 1);
}

static ULONG bit(struct BitMap *bm, ULONG x, ULONG y, ULONG p)
{
    PLANEPTR plane = bm->Planes[p];
    if (!plane || plane == (PLANEPTR)-1)
        return plane != NULL;
    return (plane[y * bm->BytesPerRow + x / 8] >> (7 - (x & 7))) & 1;
}

static void fill(struct BitMap *bm, BOOL source)
{
    ULONG x, y, p;
    for (p = 0; p < 4; p++)
        for (y = 0; y < 64; y++)
            for (x = 0; x < 128; x += 8)
            {
                UBYTE value = 0;
                ULONG b;
                for (b = 0; b < 8; b++)
                    value |= pattern(x + b, y, p, source) << (7 - b);
                bm->Planes[p][y * bm->BytesPerRow + x / 8] = value;
            }
}

static void exercise(ULONG flags)
{
    struct BitMap *src = AllocBitMap(128, 64, 4, BMF_CLEAR | flags, NULL);
    struct BitMap *dst = AllocBitMap(128, 64, 4, BMF_CLEAR | flags, NULL);
    const WORD rectangles[][4] = {{0, 0, 128, 64}, {2, 7, 85, 23}, {7, 2, 85, 23}};
    ULONG i, x, y, p, mismatches;

    check(src && dst, "allocate bitmaps");
    if (!src || !dst)
        goto out;
    fill(src, TRUE);
    for (i = 0; i < 3; i++)
    {
        WORD sx = rectangles[i][0], dx = rectangles[i][1];
        WORD width = rectangles[i][2], height = rectangles[i][3];
        UBYTE mask = i ? 5 : 15;
        fill(dst, FALSE);
        BltBitMap(src, sx, 0, dst, dx, 0, width, height, 0x80, mask, NULL);
        WaitBlit();
        mismatches = 0;
        for (p = 0; p < 4; p++)
            for (y = 0; y < 64; y++)
                for (x = 0; x < 128; x++)
                {
                    ULONG expected = pattern(x, y, p, FALSE);
                    if (x >= dx && x < dx + width && y < height && (mask & (1 << p)))
                        expected &= pattern(x + sx - dx, y, p, TRUE);
                    if (bit(dst, x, y, p) != expected)
                        mismatches++;
                }
        check(!mismatches, "AND pixels, plane mask and untouched edges");
    }
out:
    if (dst)
        FreeBitMap(dst);
    if (src)
        FreeBitMap(src);
}

int main(void)
{
    struct MsgPort *port;
    struct timerequest *request = NULL;
    struct BitMap *src = NULL, *dst = NULL;
    BOOL timer_open = FALSE;
    struct EClockVal start, end;
    ULONG frequency;

    exercise(0);
    exercise(BMF_INTERLEAVED);
    port = CreateMsgPort();
    if (port)
        request = (struct timerequest *)CreateIORequest(port, sizeof(*request));
    src = AllocBitMap(128, 64, 4, BMF_CLEAR | BMF_INTERLEAVED, NULL);
    dst = AllocBitMap(128, 64, 4, BMF_CLEAR | BMF_INTERLEAVED, NULL);
    if (src && dst && request &&
        OpenDevice(TIMERNAME, UNIT_ECLOCK, (struct IORequest *)request, 0) == 0)
    {
        timer_open = TRUE;
        TimerBase = request->tr_node.io_Device;
        frequency = ReadEClock(&start);
        BltBitMap(src, 0, 0, dst, 0, 0, 128, 64, 0x80, 15, NULL);
        WaitBlit();
        ReadEClock(&end);
        bug("PLANAR AND TIME: %lu ticks / %lu Hz\n",
            (unsigned long)(end.ev_lo - start.ev_lo), (unsigned long)frequency);
        check(frequency && end.ev_hi - start.ev_hi <= 1 &&
              end.ev_lo - start.ev_lo < frequency / 4,
              "stock CD32 AND copy completes within 250 ms");
    }
    else
        check(FALSE, "open timer and allocate performance bitmaps");
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
    bug("PLANAR AND TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
