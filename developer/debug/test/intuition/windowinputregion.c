/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: WA_InputRegion with real input: clicks sent through input.device
          go to the window whose input region they are in.

    On the Workbench screen: window BACK (plain, 320x200 at 60,60, reports
    button presses) and in front of it window FRONT (borderless, 240x140 at
    100,90, alpha surface where the screen composes): a 30 pixel translucent
    rim around an opaque inner part, opened with WA_InputRegion = the inner
    part. The pointer is moved and the left button pressed and released
    through input.device (IND_WRITEEVENT), as a mouse would do.

    Expected:
    - a click on FRONT's rim reaches BACK;
    - a click on FRONT's inner part reaches FRONT;
    - after ChangeLayerInputShape(FRONT->WLayer, NULL) the rim click reaches
      FRONT (the default: where it is seen);
    - window PLAIN (no alpha surface, so also without composition), opened
      with WA_InputRegion = its left half, takes clicks only there; a click
      in its right half reaches BACK.

    Usage: windowinputregion
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/regions.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <stdio.h>
#include <string.h>

#define RIM 30

static int failures = 0;
static struct IOStdReq *io;
static struct Screen *scr;

static void result(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static void send_event(struct InputEvent *ie)
{
    io->io_Command = IND_WRITEEVENT;
    io->io_Data = ie;
    io->io_Length = sizeof(*ie);
    DoIO((struct IORequest *)io);
}

/* Pointer to (x,y) on the screen, then a left button press and release */
static void click(WORD x, WORD y)
{
    struct IEPointerPixel pp;
    struct InputEvent ie;

    memset(&ie, 0, sizeof(ie));
    pp.iepp_Screen = scr;
    pp.iepp_Position.X = x;
    pp.iepp_Position.Y = y;
    ie.ie_Class = IECLASS_NEWPOINTERPOS;
    ie.ie_SubClass = IESUBCLASS_PIXEL;
    ie.ie_EventAddress = &pp;
    send_event(&ie);
    Delay(2);

    memset(&ie, 0, sizeof(ie));
    ie.ie_Class = IECLASS_RAWMOUSE;
    ie.ie_Code = IECODE_LBUTTON;
    ie.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE | IEQUALIFIER_LEFTBUTTON;
    send_event(&ie);
    Delay(2);
    ie.ie_Code = IECODE_LBUTTON | IECODE_UP_PREFIX;
    ie.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE;
    send_event(&ie);
    Delay(10);
}

/* How many SELECTDOWN presses w has had since the last call */
static int presses(struct Window *w)
{
    struct IntuiMessage *m;
    int n = 0;

    while ((m = (struct IntuiMessage *)GetMsg(w->UserPort)))
    {
        if (m->Class == IDCMP_MOUSEBUTTONS && m->Code == SELECTDOWN)
            n++;
        ReplyMsg((struct Message *)m);
    }
    return n;
}

static void expect(WORD x, WORD y, struct Window *want, struct Window *back, struct Window *other,
                   const char *what)
{
    int pb, po;
    char buf[160];

    presses(back); presses(other);
    click(x, y);
    pb = presses(back);
    po = presses(other);
    snprintf(buf, sizeof(buf), "click at %d,%d: back %d, front %d (%s)", x, y, pb, po, what);
    result(want == back ? (pb == 1 && po == 0) : (po == 1 && pb == 0), buf);
}

static struct Region *rect_region(WORD x0, WORD y0, WORD x1, WORD y1)
{
    struct Region *r = NewRegion();
    struct Rectangle rect = { x0, y0, x1, y1 };

    if (r && !OrRectRegion(r, &rect))
    {
        DisposeRegion(r);
        r = NULL;
    }
    return r;
}

int main(void)
{
    struct MsgPort *port;
    struct Window *back = NULL, *front = NULL, *plain = NULL;
    struct Region *inner, *left;
    UBYTE *row;
    int x, y;

    port = CreateMsgPort();
    io = port ? (struct IOStdReq *)CreateIORequest(port, sizeof(*io)) : NULL;
    if (!io || OpenDevice("input.device", 0, (struct IORequest *)io, 0))
    {
        printf("FAIL: input.device\n");
        return 20;
    }
    scr = LockPubScreen(NULL);
    inner = rect_region(RIM, RIM, 240 - RIM - 1, 140 - RIM - 1);
    left = rect_region(0, 0, 59, 79);
    if (scr && inner && left)
    {
        back = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr, WA_Left, 60, WA_Top, 60, WA_Width, 320, WA_Height, 200,
                              WA_Title, (IPTR)"back", WA_IDCMP, IDCMP_MOUSEBUTTONS, WA_RMBTrap, TRUE, TAG_DONE);
        front = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr, WA_Left, 100, WA_Top, 90, WA_Width, 240, WA_Height, 140,
                               WA_Borderless, TRUE, WA_AlphaSurface, TRUE, WA_InputRegion, (IPTR)inner,
                               WA_IDCMP, IDCMP_MOUSEBUTTONS, WA_RMBTrap, TRUE, TAG_DONE);
        plain = OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr, WA_Left, 450, WA_Top, 90, WA_Width, 120, WA_Height, 80,
                               WA_Borderless, TRUE, WA_InputRegion, (IPTR)left,
                               WA_IDCMP, IDCMP_MOUSEBUTTONS, WA_RMBTrap, TRUE, TAG_DONE);
    }
    /* Intuition's layers keep copies */
    if (inner) DisposeRegion(inner);
    if (left) DisposeRegion(left);

    if (!back || !front || !plain)
    {
        printf("FAIL: cannot open windows\n");
        failures++;
        goto done;
    }
    printf("front %s\n", (front->WLayer->Flags & LAYERCOMPOSITED) ? "composited" : "not composited");

    /* the rim translucent, the inside opaque */
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
    FillPixelArray(plain->RPort, 0, 0, 120, 80, 0x00A000);
    Delay(25);

    expect(110, 100, back, back, front, "front's rim, outside its input region");
    expect(220, 160, front, back, front, "front's inner part");
    result(ChangeLayerInputShape(front->WLayer, NULL), "ChangeLayerInputShape(front, NULL)");
    expect(110, 100, front, back, front, "front's rim with the default input shape");

    /* plain lies outside back: its misses go to the screen, so only check plain */
    presses(plain);
    click(470, 120);
    result(presses(plain) == 1, "plain, inside its input region (left half), gets the click");
    click(550, 120);
    result(presses(plain) == 0, "plain, outside its input region (right half), does not");

done:
    if (plain) CloseWindow(plain);
    if (front) CloseWindow(front);
    if (back) CloseWindow(back);
    if (scr) UnlockPubScreen(NULL, scr);
    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);
    printf("%s: windowinputregion, %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
