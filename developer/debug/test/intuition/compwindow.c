/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Opens two windows on the Workbench screen: an ordinary one and,
          partly over it, one with WA_Opaqueness (composited layer, see
          layers.library LA_Composited). Reports the layer flags and whether
          the composited window kept its content where the two overlap.

    Usage: compwindow [DELAY=seconds] [OPACITY=n] [ALPHA]
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <intuition/intuition.h>
#include <graphics/clip.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>

static void fill(struct Window *w, ULONG rgb, ULONG border)
{
    struct RastPort *rp = w->RPort;
    WORD x0 = w->BorderLeft, y0 = w->BorderTop;
    WORD x1 = w->Width - w->BorderRight - 1, y1 = w->Height - w->BorderBottom - 1;

    FillPixelArray(rp, x0, y0, x1 - x0 + 1, y1 - y0 + 1, border);
    FillPixelArray(rp, x0 + 12, y0 + 12, x1 - x0 - 23, y1 - y0 - 23, rgb);
}

/*
 * Per pixel alpha: pens render with alpha 0 into an alpha surface, so the
 * content is written as ARGB. Left half alpha 0x40, right half opaque, a
 * fully transparent hole in the middle.
 */
static void fill_alpha(struct Window *w)
{
    WORD x0 = w->BorderLeft, y0 = w->BorderTop;
    WORD cw = w->Width - w->BorderLeft - w->BorderRight;
    WORD ch = w->Height - w->BorderTop - w->BorderBottom;
    UBYTE *row = AllocVec(cw * 4, MEMF_ANY);
    WORD x, y;

    if (!row)
        return;
    for (y = 0; y < ch; y++)
    {
        for (x = 0; x < cw; x++)
        {
            BOOL hole = (x > cw / 2 - 30 && x < cw / 2 + 30 && y > ch / 2 - 30 && y < ch / 2 + 30);
            BOOL edge = (x < 12 || y < 12 || x >= cw - 12 || y >= ch - 12);
            row[x * 4 + 0] = hole ? 0x00 : (x < cw / 2 ? 0x40 : 0xFF);  /* A */
            row[x * 4 + 1] = edge ? 0xFF : 0x00;                           /* R */
            row[x * 4 + 2] = edge ? 0x00 : 0xFF;                           /* G */
            row[x * 4 + 3] = edge ? 0xFF : 0x00;                           /* B */
        }
        WritePixelArray(row, 0, 0, cw * 4, w->RPort, x0, y0 + y, cw, 1, RECTFMT_ARGB);
    }
    FreeVec(row);
}

int main(void)
{
    IPTR args[3] = { 0, 0, 0 };
    struct RDArgs *rda;
    LONG delay = 8, opacity = 160;
    BOOL alpha = FALSE;
    struct Window *a, *b;
    int fails = 0;
    UBYTE pix[4];
    ULONG got;

    rda = ReadArgs("DELAY/N,OPACITY/N,ALPHA/S", args, NULL);
    if (rda)
    {
        if (args[0]) delay   = *(LONG *)args[0];
        if (args[1]) opacity = *(LONG *)args[1];
        alpha = args[2] ? TRUE : FALSE;
        FreeArgs(rda);
    }

    a = OpenWindowTags(NULL,
        WA_Left, 120, WA_Top, 120, WA_Width, 360, WA_Height, 240,
        WA_Title, (IPTR)"plain window", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_Activate, FALSE, TAG_END);
    if (!a) { printf("FAIL: plain window\n"); return RETURN_FAIL; }
    fill(a, 0xFF0000, 0xFFFF00);

    b = OpenWindowTags(NULL,
        WA_Left, 300, WA_Top, 220, WA_Width, 320, WA_Height, 220,
        WA_Title, (IPTR)"composited window", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_Opaqueness, opacity,
        alpha ? WA_AlphaSurface : TAG_IGNORE, TRUE,
        WA_Activate, TRUE, TAG_END);
    if (!b) { printf("FAIL: composited window\n"); CloseWindow(a); return RETURN_FAIL; }
    if (alpha)
        fill_alpha(b);
    else
        fill(b, 0x00FF00, 0xFF00FF);

    printf("plain layer flags 0x%x, composited layer flags 0x%x (LAYERCOMPOSITED %s)\n",
           a->WLayer->Flags, b->WLayer->Flags,
           (b->WLayer->Flags & LAYERCOMPOSITED) ? "set" : "NOT set");
    if (!(b->WLayer->Flags & LAYERCOMPOSITED))
    {
        printf("NOTE: screen cannot compose; plain smart refresh window instead\n");
    }

    /* the composited window's own content is intact under the overlap */
    ReadPixelArray(pix, 0, 0, 4, b->RPort, b->BorderLeft + 40, b->BorderTop + 40, 1, 1, RECTFMT_ARGB);
    got = ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
    if (got != 0x00FF00)
    {
        printf("FAIL: composited window content %06lx, wanted 00ff00\n", (unsigned long)got);
        fails++;
    }
    /* the plain window's content under the composited one is intact (not clobbered) */
    ReadPixelArray(pix, 0, 0, 4, a->RPort, 300, 180, 1, 1, RECTFMT_ARGB);
    got = ((ULONG)pix[1] << 16) | ((ULONG)pix[2] << 8) | pix[3];
    if (got != 0xFF0000)
    {
        printf("FAIL: plain window content under overlap %06lx, wanted ff0000\n", (unsigned long)got);
        fails++;
    }

    Delay(delay * 50);

    MoveWindow(b, -100, -60);
    Delay(delay * 50);
    WindowToBack(b);
    Delay(delay * 50);
    WindowToFront(b);
    Delay(delay * 50);

    CloseWindow(b);
    CloseWindow(a);
    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
