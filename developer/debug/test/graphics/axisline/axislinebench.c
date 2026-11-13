/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <devices/timer.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/timer.h>

struct Device *TimerBase;

int main(void)
{
    struct BitMap *bitmap = AllocBitMap(320, 32, 8,
        BMF_CLEAR | BMF_INTERLEAVED, NULL);
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *request = port ?
        (struct timerequest *)CreateIORequest(port, sizeof(*request)) : NULL;
    struct RastPort rp;
    struct EClockVal start, end;
    BOOL opened = FALSE;
    ULONG frequency, i, failures = 0;

    if (bitmap && request && OpenDevice(TIMERNAME, UNIT_ECLOCK,
        (struct IORequest *)request, 0) == 0)
    {
        opened = TRUE;
        TimerBase = request->tr_node.io_Device;
        InitRastPort(&rp);
        rp.BitMap = bitmap;
        SetAPen(&rp, 21);
        frequency = ReadEClock(&start);
        for (i = 0; i < 16; i++)
        {
            Move(&rp, 0, i);
            Draw(&rp, 319, i);
        }
        WaitBlit();
        ReadEClock(&end);
        bug("AXIS LINE TIME: %lu ticks / %lu Hz\n",
            (unsigned long)(end.ev_lo - start.ev_lo), (unsigned long)frequency);
        if (!frequency || end.ev_hi - start.ev_hi > 1 ||
            end.ev_lo - start.ev_lo >= frequency / 4)
            failures++;
    }
    else
        failures++;
    if (opened)
        CloseDevice((struct IORequest *)request);
    if (request)
        DeleteIORequest((struct IORequest *)request);
    if (port)
        DeleteMsgPort(port);
    if (bitmap)
        FreeBitMap(bitmap);
    bug("AXIS LINE BENCH TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
