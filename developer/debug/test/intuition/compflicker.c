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

    Checking it needs captures of the displayed picture taken from outside
    the system (for example QEMU's screendump, several per second): the
    window's area must be the same in every one of them.

    Usage: compflicker [SECONDS=n] [OPACITY=n] [OPAQUE]
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <intuition/intuition.h>
#include <graphics/clip.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 80

int main(void)
{
    IPTR args[3] = { 0, 0, 0 };
    struct RDArgs *rda = ReadArgs("SECONDS/N,OPACITY/N,OPAQUE/S", args, NULL);
    int seconds = (rda && args[0]) ? *(LONG *)args[0] : 20;
    int alpha = (rda && args[1]) ? *(LONG *)args[1] : 128, i;

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

    int writes = 0, rounds = seconds * 50;
    for (i = 0; i < rounds && !(SetSignal(0, 0) & SIGBREAKF_CTRL_C); i++)
    {
        WritePixelArray(px, 0, 0, W * 4, w->RPort, 0, 0, W, H, RECTFMT_ARGB);
        writes++;
        Delay(1);
    }
    printf("writes=%d\n", writes);
    CloseWindow(w);
    free(px);
    if (rda) FreeArgs(rda);
    return 0;
}
