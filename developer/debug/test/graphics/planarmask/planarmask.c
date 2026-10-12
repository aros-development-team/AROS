/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Masked copies of ordinary planar bitmaps must preserve surrounding pixels
    and finish within a small fraction of a second on a stock CD32.
*/
#include <aros/debug.h>
#include <devices/timer.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/timer.h>
#include <string.h>

struct Device *TimerBase;
static ULONG failures;

static void check(BOOL okay, CONST_STRPTR label)
{
    if (!okay)
    {
        bug("PLANAR MASK FAIL: %s\n", label);
        failures++;
    }
}

static ULONG bit(PLANEPTR plane, ULONG stride, ULONG x, ULONG y)
{
    if (!plane)
        return 0;
    if (plane == (PLANEPTR)-1)
        return 1;
    return (plane[y * stride + x / 8] >> (7 - (x & 7))) & 1;
}

static void exercise(struct BitMap *src, struct BitMap *dst, PLANEPTR mask,
                     WORD sx, WORD sy, WORD dx, WORD dy,
                     WORD width, WORD height, UBYTE minterm)
{
    struct RastPort rp;
    ULONG p, x, y, mismatches = 0;

    InitRastPort(&rp);
    rp.BitMap = dst;
    for (p = 0; p < dst->Depth; p++)
        memset(dst->Planes[p], 0x5a, dst->BytesPerRow * dst->Rows);
    if (mask)
        BltMaskBitMapRastPort(src, sx, sy, &rp, dx, dy, width, height, minterm, mask);
    else
        BltBitMap(src, sx, sy, dst, dx, dy, width, height, minterm, 0xff, NULL);
    WaitBlit();

    for (p = 0; p < dst->Depth; p++)
        for (y = 0; y < dst->Rows; y++)
            for (x = 0; x < (ULONG)dst->BytesPerRow * 8; x++)
            {
                ULONG expected = (0x5a >> (7 - (x & 7))) & 1;
                if (x >= dx && x < dx + width && y >= dy && y < dy + height &&
                    (!mask || bit(mask, src->BytesPerRow, sx + x - dx, sy + y - dy)))
                {
                    ULONG source = bit(src->Planes[p], src->BytesPerRow,
                                       sx + x - dx, sy + y - dy);
                    /* Both 0xc0 and 0xe0 replace through the supplied mask.
                       A set destination bit must clear when source is zero. */
                    expected = source;
                }
                if (bit(dst->Planes[p], dst->BytesPerRow, x, y) != expected)
                    mismatches++;
            }
    bug("PLANAR MASK CASE: source %d,%d destination %d,%d %dx%d minterm %02x: %lu mismatches\n",
        sx, sy, dx, dy, width, height, minterm, (unsigned long)mismatches);
    check(mismatches == 0, "masked pixels and untouched edges match");
}

int main(void)
{
    struct BitMap src, dst, narrow, constantdst;
    struct Screen *screen = NULL;
    struct MsgPort *port = NULL;
    struct timerequest *request = NULL;
    PLANEPTR allocation = NULL, mask;
    ULONG p, i, frequency;
    const ULONG size = 24 * 80;
    struct EClockVal start, end;
    struct RastPort rp;
    BOOL timer_open = FALSE;

    allocation = AllocMem(size * 17, MEMF_CHIP | MEMF_CLEAR);
    if (!allocation)
        return 20;
    InitBitMap(&src, 4, 192, 80);
    InitBitMap(&dst, 4, 192, 80);
    for (p = 0; p < 4; p++)
    {
        src.Planes[p] = allocation + size * p;
        dst.Planes[p] = allocation + size * (p + 4);
        for (i = 0; i < size; i++)
            src.Planes[p][i] = (i * 37 + p * 61) ^ (i >> 3);
    }
    mask = allocation + size * 16;
    for (i = 0; i < size; i++)
        mask[i] = (i & 1) ? 0x69 : 0xb6;

    screen = OpenScreenTags(NULL, SA_Width, 320, SA_Height, 256,
                            SA_Depth, 4, SA_Quiet, TRUE, TAG_DONE);
    check(screen != NULL, "open native screen");
    if (!screen)
        goto out;

    exercise(&src, &dst, mask, 0, 2, 16, 4, 85, 23, 0xc0);
    exercise(&src, &dst, mask, 2, 2, 7, 4, 85, 23, 0xc0);
    exercise(&src, &dst, mask, 7, 2, 2, 4, 85, 23, 0xc0);
    exercise(&src, screen->RastPort.BitMap, mask, 2, 2, 7, 4, 85, 23, 0xc0);
    exercise(&src, &dst, mask, 0, 2, 16, 4, 85, 23, 0xe0);
    exercise(&src, &dst, mask, 2, 2, 7, 4, 85, 23, 0xe0);
    exercise(&src, &dst, mask, 7, 2, 2, 4, 85, 23, 0xe0);
    /* Simon draws 16-pixel-wide, three-row strips at positions which span
       two destination words. Exercise that source stride, not just a
       subrectangle of a wide bitmap. */
    InitBitMap(&narrow, 4, 16, 3);
    for (p = 0; p < 4; p++)
        narrow.Planes[p] = src.Planes[p];
    exercise(&narrow, &dst, mask, 0, 0, 152, 12, 16, 3, 0xe0);
    exercise(&narrow, &dst, mask, 0, 0, 159, 12, 16, 3, 0xe0);
    InitBitMap(&narrow, 8, 16, 18);
    InitBitMap(&constantdst, 8, 192, 80);
    for (p = 0; p < 4; p++)
        narrow.Planes[p] = src.Planes[p];
    narrow.Planes[4] = NULL;
    narrow.Planes[5] = (PLANEPTR)-1;
    narrow.Planes[6] = NULL;
    narrow.Planes[7] = NULL;
    for (p = 0; p < 8; p++)
        constantdst.Planes[p] = allocation + size * (p + 8);
    exercise(&narrow, &constantdst, mask, 0, 0, 152, 12, 16, 18, 0xe0);
    exercise(&narrow, &constantdst, NULL, 0, 0, 152, 12, 16, 18, 0xc0);

    port = CreateMsgPort();
    if (port)
        request = (struct timerequest *)CreateIORequest(port, sizeof(*request));
    if (request && OpenDevice(TIMERNAME, UNIT_ECLOCK,
                             (struct IORequest *)request, 0) == 0)
    {
        timer_open = TRUE;
        TimerBase = request->tr_node.io_Device;
        InitRastPort(&rp);
        rp.BitMap = &dst;
        frequency = ReadEClock(&start);
        BltMaskBitMapRastPort(&src, 0, 0, &rp, 0, 0, 128, 64, 0xc0, mask);
        WaitBlit();
        ReadEClock(&end);
        bug("PLANAR MASK TIME: %lu ticks / %lu Hz\n",
            (unsigned long)(end.ev_lo - start.ev_lo), (unsigned long)frequency);
        /* 250 ms leaves ample margin for driver setup and interrupts; the
           per-pixel fallback takes substantially longer on the same machine. */
        check(frequency && end.ev_hi - start.ev_hi <= 1 &&
              end.ev_lo - start.ev_lo < frequency / 4,
              "stock CD32 masked copy completes within 250 ms");
    }
    else
        check(FALSE, "open timer for performance assertion");

out:
    if (timer_open)
        CloseDevice((struct IORequest *)request);
    if (request)
        DeleteIORequest((struct IORequest *)request);
    if (port)
        DeleteMsgPort(port);
    if (screen)
        CloseScreen(screen);
    FreeMem(allocation, size * 17);
    bug("PLANAR MASK TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
