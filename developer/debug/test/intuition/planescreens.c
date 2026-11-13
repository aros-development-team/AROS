/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Open several custom screens, then move, depth-arrange and draw into
          them with pauses between the steps. Exercises the display compositor's
          hybrid plane assignment: on a display with overlay scan-out planes
          (hosted X11 emulates two) the frontmost screens are scanned out by
          planes and the rest are composed, and every step below changes that
          assignment or must reach the display through a plane. A host-side
          observer (harness/hosted/xplanes for X11) checks the plane windows;
          this program checks what Intuition reports.

    Usage: planescreens [DELAY=seconds] [COUNT=n]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/screens.h>
#include <hidd/gfx.h>
#include <stdio.h>

#define MAXSCREENS 4

static const ULONG colours[MAXSCREENS][3] =
{
    { 0xFFFFFFFF, 0x00000000, 0x00000000 },     /* red    */
    { 0x00000000, 0xFFFFFFFF, 0x00000000 },     /* green  */
    { 0x00000000, 0x00000000, 0xFFFFFFFF },     /* blue   */
    { 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },     /* yellow */
};

static int fails = 0;

static void check_pos(struct Screen *s, const char *name, WORD x, WORD y)
{
    if ((s->LeftEdge != x) || (s->TopEdge != y))
    {
        printf("FAIL: %s at %d,%d, expected %d,%d\n", name, s->LeftEdge, s->TopEdge, x, y);
        fails++;
    }
    else
        printf("PASS: %s at %d,%d\n", name, x, y);
}

/* Each screen: solid colour, a white 40x40 marker at (20 + 60 * i, 20) */
static void paint(struct Screen *s, int i, UBYTE markerpen)
{
    struct RastPort *rp = &s->RastPort;

    SetRGB32(&s->ViewPort, 0, colours[i][0], colours[i][1], colours[i][2]);
    SetRGB32(&s->ViewPort, 1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    SetRGB32(&s->ViewPort, 2, 0x00000000, 0x00000000, 0x00000000);
    /* Truecolor screens keep the pixels they were cleared with: repaint in the new pen 0 */
    SetRast(rp, 0);
    SetAPen(rp, markerpen);
    RectFill(rp, 20 + 60 * i, 20, 20 + 60 * i + 39, 59);
}

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda;
    LONG delay = 6, count = 3;
    WORD dy;
    struct Screen *s[MAXSCREENS] = { NULL };
    int i;

    rda = ReadArgs("DELAY/N,COUNT/N", args, NULL);
    if (rda)
    {
        if (args[0]) delay = *(LONG *)args[0];
        if (args[1]) count = *(LONG *)args[1];
        FreeArgs(rda);
    }
    if (count < 1) count = 1;
    if (count > MAXSCREENS) count = MAXSCREENS;

    /* Make sure Workbench exists (and stays behind) before our screens open in front of it */
    {
        struct Screen *wb = LockPubScreen("Workbench");
        if (wb)
        {
            printf("Workbench: %dx%d mode 0x%08lx obj %p\n", wb->Width, wb->Height, (unsigned long)GetVPModeID(&wb->ViewPort),
                HIDD_BM_OBJ(wb->RastPort.BitMap));
            UnlockPubScreen(NULL, wb);
        }
    }

    /*
     * Open back to front, each new screen in front of the previous one, so the
     * stack is S0, S1, ..., Workbench (SA_Behind would put them behind Workbench).
     */
    for (i = count - 1; i >= 0; i--)
    {
        s[i] = OpenScreenTags(NULL,
            SA_Depth,     24,
            SA_Type,      CUSTOMSCREEN,
            SA_Quiet,     TRUE,
            SA_ShowTitle, FALSE,
            SA_Title,     (IPTR)"planescreens",
            TAG_END);
        if (!s[i])
        {
            printf("FAIL: cannot open screen %d\n", i);
            fails++;
            break;
        }
        paint(s[i], i, 1);
    }
    if (i >= 0)
    {
        for (i = count - 1; i >= 0; i--)
            if (s[i])
                CloseScreen(s[i]);
        return RETURN_FAIL;
    }

    for (i = 0; i < count; i++)
        printf("S%d: %dx%d mode 0x%08lx bitmap %ldx%ld obj %p\n", i, s[i]->Width, s[i]->Height,
            (unsigned long)GetVPModeID(&s[i]->ViewPort),
            (long)GetBitMapAttr(s[i]->RastPort.BitMap, BMA_WIDTH), (long)GetBitMapAttr(s[i]->RastPort.BitMap, BMA_HEIGHT),
            HIDD_BM_OBJ(s[i]->RastPort.BitMap));
    printf("STEP 0: %ld screens open, S0 (red) in front\n", (long)count);
    Delay(delay * 50);

    /* Screens drag vertically, as on AmigaOS; the display is as tall as the front screen */
    dy = s[0]->Height / 2;
    MoveScreen(s[0], 0, dy);
    printf("STEP 1: MoveScreen S0 +0,+%d\n", dy);
    check_pos(s[0], "S0", 0, dy);
    Delay(delay * 50);

    if (count > 1)
    {
        MoveScreen(s[1], 0, dy / 2);
        printf("STEP 2: MoveScreen S1 +0,+%d\n", dy / 2);
        check_pos(s[1], "S1", 0, dy / 2);
        Delay(delay * 50);
    }

    ScreenToBack(s[0]);
    printf("STEP 3: ScreenToBack S0\n");
    Delay(delay * 50);

    if (count > 1)
    {
        paint(s[1], 1, 2);
        printf("STEP 4: S1 marker repainted black (update through the plane)\n");
        Delay(delay * 50);
    }

    MoveScreen(s[0], 0, -dy);
    ScreenToFront(s[0]);
    printf("STEP 5: MoveScreen S0 +0,-%d, ScreenToFront S0\n", dy);
    check_pos(s[0], "S0", 0, 0);
    Delay(delay * 50);

    if (count > 1)
    {
        MoveScreen(s[1], 0, -(dy / 2));
        printf("STEP 6: MoveScreen S1 +0,-%d\n", dy / 2);
        check_pos(s[1], "S1", 0, 0);
        Delay(delay * 50);
    }

    for (i = count - 1; i >= 0; i--)
        CloseScreen(s[i]);
    printf("STEP 7: all screens closed\n");
    Delay(50);

    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
