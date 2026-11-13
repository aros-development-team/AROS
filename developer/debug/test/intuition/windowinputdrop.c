/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: WA_InputRegion and Workbench drops: an icon dropped on a window's
          translucent rim, outside its input region, goes to the window
          behind it.

    Opens two AppWindows on the Workbench screen: BACK (320x200 at 420,120)
    and, in front of it, FRONT (borderless, 240x140 at 460,150, alpha
    surface: a 30 pixel translucent rim around an opaque inner part, with
    WA_InputRegion = the inner part). Then waits for icons to be dropped,
    up to SECONDS (default 60) or COUNT drops (default 2), and prints which
    window each drop reached and where.

    Drag a Workbench icon onto FRONT's rim (e.g. 470,160): it must reach
    BACK. Drag one onto FRONT's inner part (e.g. 580,220): it must reach
    FRONT.

    Usage: windowinputdrop [SECONDS=n] [COUNT=n]
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/regions.h>
#include <workbench/workbench.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/workbench.h>
#include <proto/cybergraphics.h>

#include <stdio.h>

#define RIM 30

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda = ReadArgs("SECONDS/N,COUNT/N", args, NULL);
    LONG seconds = (rda && args[0]) ? *(LONG *)args[0] : 60, count = (rda && args[1]) ? *(LONG *)args[1] : 2;
    struct Screen *scr = LockPubScreen(NULL);
    struct MsgPort *port = CreateMsgPort();
    struct Region *inner = NewRegion();
    struct Rectangle r = { RIM, RIM, 240 - RIM - 1, 140 - RIM - 1 };
    struct Window *back = NULL, *front = NULL;
    struct AppWindow *aback = NULL, *afront = NULL;
    UBYTE *row;
    LONG x, y, got = 0, ticks;

    if (scr && port && inner && OrRectRegion(inner, &r))
    {
        back = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr, WA_Left, 420, WA_Top, 120, WA_Width, 320, WA_Height, 200,
                              WA_Title, (IPTR)"back (AppWindow)", TAG_DONE);
        front = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr, WA_Left, 460, WA_Top, 150, WA_Width, 240, WA_Height, 140,
                               WA_Borderless, TRUE, WA_AlphaSurface, TRUE, WA_InputRegion, (IPTR)inner, TAG_DONE);
    }
    if (inner) DisposeRegion(inner);
    if (back) aback = AddAppWindow(1, 0, back, port, TAG_DONE);
    if (front) afront = AddAppWindow(2, 0, front, port, TAG_DONE);
    if (!aback || !afront)
    {
        printf("FAIL: cannot open the AppWindows\n");
        goto done;
    }
    if ((row = AllocVec(240 * 4, MEMF_ANY)))
    {
        for (y = 0; y < 140; y++)
        {
            for (x = 0; x < 240; x++)
            {
                BOOL in = x >= RIM && x < 240 - RIM && y >= RIM && y < 140 - RIM;

                row[x * 4 + 0] = in ? 0xFF : 0x50;
                row[x * 4 + 1] = in ? 0x30 : 0;
                row[x * 4 + 2] = in ? 0x80 : 0;
                row[x * 4 + 3] = in ? 0xC0 : 0;
            }
            WritePixelArray(row, 0, 0, 240 * 4, front->RPort, 0, y, 240, 1, RECTFMT_ARGB);
        }
        FreeVec(row);
    }
    printf("ready: front %s; drop icons on its rim (470,160) and inner part (580,220)\n",
           (front->WLayer->Flags & LAYERCOMPOSITED) ? "composited" : "not composited");
    fflush(stdout);

    for (ticks = 0; ticks < seconds * 50 && got < count; ticks += 5)
    {
        struct AppMessage *m;

        while ((m = (struct AppMessage *)GetMsg(port)))
        {
            printf("drop %ld: %s window, at %d,%d (window relative), %ld icon(s)\n", (long)++got,
                   m->am_ID == 1 ? "BACK" : "FRONT", m->am_MouseX, m->am_MouseY, (long)m->am_NumArgs);
            fflush(stdout);
            ReplyMsg((struct Message *)m);
        }
        Delay(5);
    }
    printf("%ld drop(s)\n", (long)got);

done:
    if (afront) RemoveAppWindow(afront);
    if (aback) RemoveAppWindow(aback);
    if (front) CloseWindow(front);
    if (back) CloseWindow(back);
    if (port) DeleteMsgPort(port);
    if (scr) UnlockPubScreen(NULL, scr);
    if (rda) FreeArgs(rda);
    return 0;
}
