/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Alpha screen composition test.

    Opens an opaque screen with colour bars, then an ARGB screen in front of
    it that asks the display compositor to alpha-blend it (SA_CompositingFlags
    with COMPF_ALPHA). The front screen is painted with known coverage values
    so the composited result can be checked by eye or from a screen capture:

        rows 0..1/4    : alpha 0    (bars fully visible)
        rows 1/4..1/2  : alpha 128, white (bars at half brightness towards white)
        rows 1/2..3/4  : alpha 255, magenta (bars hidden)
        rows 3/4..1    : horizontal alpha ramp 0..255, black
        plus a vertical stripe of alpha 64 green down the middle.

    By default the screens use the Workbench screen's mode and size, so no
    display mode switch is involved (some drivers fail to come back from a
    modeset). WIDTH/HEIGHT pick a different mode via BestModeID().

    Usage: alphascreen [WIDTH=n] [HEIGHT=n] [DELAY=seconds] [BEHIND]
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/gfx.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <string.h>
#include <stdio.h>

#define ARG_TEMPLATE "WIDTH/N,HEIGHT/N,DELAY/N,BEHIND/S"
enum { ARG_WIDTH, ARG_HEIGHT, ARG_DELAY, ARG_BEHIND, NUM_ARGS };

static const ULONG bar_colours[8] =
{
    0xFFFFFF, 0xFFFF00, 0x00FFFF, 0x00FF00, 0xFF00FF, 0xFF0000, 0x0000FF, 0x000000
};

static void fill_bars(struct RastPort *rp, ULONG width, ULONG height)
{
    ULONG i;

    for (i = 0; i < 8; i++)
    {
        ULONG x0 = width * i / 8;
        ULONG x1 = width * (i + 1) / 8 - 1;

        FillPixelArray(rp, x0, 0, x1 - x0 + 1, height, bar_colours[i]);
    }
}

static UBYTE *make_alpha_image(ULONG width, ULONG height)
{
    UBYTE *img = AllocVec(width * height * 4, MEMF_ANY | MEMF_CLEAR);
    ULONG x, y;

    if (!img)
        return NULL;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ULONG a, rgb;

            if (y < height / 4)
            {
                a = 0; rgb = 0x000000;
            }
            else if (y < height / 2)
            {
                a = 128; rgb = 0xFFFFFF;
            }
            else if (y < height * 3 / 4)
            {
                a = 255; rgb = 0xFF00FF;
            }
            else
            {
                a = (x * 255) / (width - 1); rgb = 0x000000;
            }

            if ((x >= width * 15 / 32) && (x < width * 17 / 32))
            {
                a = 64; rgb = 0x00FF00;
            }

            /* RECTFMT_ARGB is a byte format: A, R, G, B in memory order */
            {
                UBYTE *p = img + (y * width + x) * 4;

                p[0] = a;
                p[1] = (rgb >> 16) & 0xFF;
                p[2] = (rgb >> 8) & 0xFF;
                p[3] = rgb & 0xFF;
            }
        }
    }

    return img;
}

int main(void)
{
    IPTR args[NUM_ARGS] = { 0, 0, 0, 0 };
    struct RDArgs *rda;
    ULONG width = 0, height = 0, delay = 20;
    BOOL behind = FALSE;
    ULONG modeid = INVALID_ID;
    struct Screen *wb;
    struct Screen *back = NULL, *front = NULL;
    struct BitMap *frontbm = NULL;
    UBYTE *img;
    int rc = RETURN_FAIL;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (rda)
    {
        if (args[ARG_WIDTH])  width  = *(IPTR *)args[ARG_WIDTH];
        if (args[ARG_HEIGHT]) height = *(IPTR *)args[ARG_HEIGHT];
        if (args[ARG_DELAY])  delay  = *(IPTR *)args[ARG_DELAY];
        behind = args[ARG_BEHIND] ? TRUE : FALSE;
        FreeArgs(rda);
    }

    if (width == 0 || height == 0)
    {
        /* Same mode and size as Workbench: no modeset needed */
        wb = LockPubScreen(NULL);
        if (wb)
        {
            modeid = GetVPModeID(&wb->ViewPort);
            width  = wb->Width;
            height = wb->Height;
            UnlockPubScreen(NULL, wb);
        }
        if (modeid == INVALID_ID)
        {
            width = 640;
            height = 480;
        }
    }
    if (modeid == INVALID_ID)
        modeid = BestModeID(BIDTAG_NominalWidth, width, BIDTAG_NominalHeight, height,
                            BIDTAG_Depth, 24, TAG_DONE);
    if (modeid == INVALID_ID)
    {
        printf("No %lux%lu true colour mode\n", (unsigned long)width, (unsigned long)height);
        return RETURN_FAIL;
    }
    bug("[alphascreen] mode 0x%08lx %lux%lu\n", (unsigned long)modeid, (unsigned long)width, (unsigned long)height);

    back = OpenScreenTags(NULL,
                          SA_DisplayID, modeid,
                          SA_Width,     width,
                          SA_Height,    height,
                          SA_Depth,     24,
                          SA_Title,     (IPTR)"alphascreen: back (colour bars)",
                          SA_ShowTitle, FALSE,
                          SA_Quiet,     TRUE,
                          TAG_DONE);
    if (!back)
    {
        printf("Cannot open back screen\n");
        return RETURN_FAIL;
    }
    fill_bars(&back->RastPort, width, height);

    /*
     * The front screen needs a bitmap with an alpha channel. Allocate it as an
     * ARGB32 friend of the back screen's bitmap, so it lives on the same
     * display driver and inherits its mode, then hand it to OpenScreen() as a
     * custom bitmap. graphics.library marks it compositable for the display.
     */
    frontbm = AllocBitMap(width, height, 32,
                          BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32) | BMF_CLEAR,
                          back->RastPort.BitMap);
    if (!frontbm)
    {
        printf("Cannot allocate ARGB32 bitmap\n");
        goto out;
    }
    bug("[alphascreen] front bitmap %p pixfmt %lu\n", frontbm,
        (unsigned long)GetCyberMapAttr(frontbm, CYBRMATTR_PIXFMT));

    front = OpenScreenTags(NULL,
                           SA_DisplayID,        modeid,
                           SA_Width,            width,
                           SA_Height,           height,
                           SA_Depth,            32,
                           SA_BitMap,           (IPTR)frontbm,
                           SA_CompositingFlags, COMPF_ALPHA,
                           SA_Title,            (IPTR)"alphascreen: front (alpha)",
                           SA_ShowTitle,        FALSE,
                           SA_Quiet,            TRUE,
                           behind ? SA_Behind : TAG_IGNORE, TRUE,
                           TAG_DONE);
    if (!front)
    {
        printf("Cannot open front (alpha) screen\n");
        goto out;
    }
    {
        IPTR flags = 0;
        GetAttr(SA_CompositingFlags, (Object *)front, &flags);
        bug("[alphascreen] front screen %p compositing flags 0x%lx (asked 0x%x)\n",
            front, (unsigned long)flags, COMPF_ALPHA);
        printf("front screen compositing flags 0x%lx\n", (unsigned long)flags);
        if (!(flags & COMPF_ALPHA))
        {
            printf("Display does not offer alpha composition: the front screen is shown opaque\n");
            bug("[alphascreen] COMPF_ALPHA not granted by the display\n");
        }
    }

    img = make_alpha_image(width, height);
    if (img)
    {
        WritePixelArray(img, 0, 0, width * 4, &front->RastPort,
                        0, 0, width, height, RECTFMT_ARGB);
        FreeVec(img);
        rc = RETURN_OK;
    }

    bug("[alphascreen] painted, waiting %lu s\n", (unsigned long)delay);
    Delay(delay * TICKS_PER_SECOND);

out:
    if (front)
        CloseScreen(front);
    if (frontbm)
        FreeBitMap(frontbm);
    if (back)
        CloseScreen(back);

    return rc;
}
