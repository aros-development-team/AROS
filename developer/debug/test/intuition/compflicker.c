/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: A translucent composited window that rewrites the same picture
          over and over, to show unfinished composed frames on the display.

    Nothing the window shows ever changes, so any capture of the display in
    which the window's area differs from the others is the compositor
    showing a frame that is not finished: the backdrop drawn into the
    displayed bitmap before the window is blended over it.

    The window is 320x80 at 200,200, filled with one ARGB colour of alpha
    OPACITY (default 128), and WritePixelArray()s that colour in full every
    20 ms for SECONDS (default 20). OPAQUE writes alpha 255 instead.

    FULL leaves the window as it is and instead moves a screen hidden
    behind the Workbench screen up and down by one line every 20 ms. A
    screen that is not the front one moving makes the compositor redraw the
    whole display; the window's area must still be the same in every
    capture.

    NOMEM needs a gfx.hidd built with COMPOSITOR_FAULT_INJECTION=yes. It
    checks that a redraw which cannot get its region draws and shows
    nothing, rather than an old composed picture of the area:
    1. window BACK (opaque, red) at 200,200 with the translucent window
       FRONT over it;
    2. FRONT moves to 200,400;
    3. BACK turns green;
    4. with the redraw region failing, FRONT moves back to 200,200 and
       stays there for 3 seconds.
    During step 4 the area at 200,200 must stay plain green (0,255,0): the
    move is not drawn. FRONT blended over red, about (143,48,96), is an
    unfinished buffer from step 1 shown on the display. The program prints
    each step as it starts.

    Checking it needs captures of the displayed picture taken from outside
    the system (for example QEMU's screendump, several per second).

    Usage: compflicker [SECONDS=n] [OPACITY=n] [OPAQUE] [FULL] [NOMEM]
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <intuition/intuition.h>
#include <graphics/clip.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 80

/* Name gfx.hidd's fault injection looks for (gfx_compositorclass.c) */
#define FAIL_REDRAWREGION "compositor.fail.redrawregion"

static void step(const char *what)
{
    printf("%s\n", what);
    fflush(stdout);
}

static int nomem(struct Window *front)
{
    struct SignalSemaphore *fail = AllocMem(sizeof(*fail), MEMF_PUBLIC | MEMF_CLEAR);
    struct Window *back;

    if (!fail)
        return 20;
    back = OpenWindowTags(NULL, WA_Left, 200, WA_Top, 200, WA_Width, W, WA_Height, H,
        WA_Borderless, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!back)
    {
        printf("no window\n");
        FreeMem(fail, sizeof(*fail));
        return 20;
    }
    WindowToFront(front);
    FillPixelArray(back->RPort, 0, 0, W, H, 0xFF0000);
    Delay(50);
    step("step 1: FRONT over red BACK");
    Delay(50);

    step("step 2: FRONT moves to 200,400");
    MoveWindow(front, 0, 200);
    Delay(50);

    step("step 3: BACK turns green");
    FillPixelArray(back->RPort, 0, 0, W, H, 0x00FF00);
    Delay(50);

    fail->ss_Link.ln_Name = (char *)FAIL_REDRAWREGION;
    fail->ss_Link.ln_Pri = 0;
    AddSemaphore(fail);
    step("step 4: redraw region failing; FRONT moves back to 200,200; 200,200 must stay green");
    MoveWindow(front, 0, -200);
    Delay(150);
    RemSemaphore(fail);
    step("step 4 done");

    CloseWindow(back);
    FreeMem(fail, sizeof(*fail));
    return 0;
}

int main(void)
{
    IPTR args[5] = { 0, 0, 0, 0, 0 };
    struct RDArgs *rda = ReadArgs("SECONDS/N,OPACITY/N,OPAQUE/S,FULL/S,NOMEM/S", args, NULL);
    int seconds = (rda && args[0]) ? *(LONG *)args[0] : 20;
    int alpha = (rda && args[1]) ? *(LONG *)args[1] : 128, i, rc = 0;

    if (rda && args[2])
        alpha = 255;
    ULONG *px = malloc(W * H * 4);
    if (!px)
    {
        if (rda) FreeArgs(rda);
        return 20;
    }
    /* ARGB in memory order A,R,G,B: a mid blue. */
    for (i = 0; i < W * H; i++)
        ((UBYTE *)&px[i])[0] = (UBYTE)alpha, ((UBYTE *)&px[i])[1] = 0x20, ((UBYTE *)&px[i])[2] = 0x60, ((UBYTE *)&px[i])[3] = 0xc0;

    struct Window *w = OpenWindowTags(NULL, WA_Left, 200, WA_Top, 200, WA_Width, W, WA_Height, H,
        WA_Borderless, TRUE, WA_Activate, FALSE, WA_AlphaSurface, TRUE, TAG_DONE);
    if (!w)
    {
        printf("no window\n");
        free(px);
        if (rda) FreeArgs(rda);
        return 20;
    }
    printf("composited=%d alpha=%d\n", (w->WLayer->Flags & LAYERCOMPOSITED) ? 1 : 0, alpha);
    WritePixelArray(px, 0, 0, W * 4, w->RPort, 0, 0, W, H, RECTFMT_ARGB);

    if (rda && args[4])
        rc = nomem(w);
    else
    {
        struct Screen *hidden = NULL;
        int count = 0, rounds = seconds * 50;

        if (rda && args[3])
        {
            hidden = OpenScreenTags(NULL, SA_LikeWorkbench, TRUE, SA_Behind, TRUE, SA_Quiet, TRUE,
                SA_Title, (IPTR)"compflicker", TAG_DONE);
            if (!hidden)
            {
                printf("no screen\n");
                rc = 20;
                rounds = 0;
            }
        }
        for (i = 0; i < rounds && !(SetSignal(0, 0) & SIGBREAKF_CTRL_C); i++)
        {
            if (hidden)
                MoveScreen(hidden, 0, (i & 1) ? -1 : 1);
            else
                WritePixelArray(px, 0, 0, W * 4, w->RPort, 0, 0, W, H, RECTFMT_ARGB);
            count++;
            Delay(1);
        }
        printf("%s=%d\n", hidden ? "moves" : "writes", count);
        if (hidden)
            CloseScreen(hidden);
    }
    CloseWindow(w);
    free(px);
    if (rda) FreeArgs(rda);
    return rc;
}
