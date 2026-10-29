/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native Raspberry framebuffer regression. Run on the RGBX framebuffer
    HIDD, independently of any emulated application or CPU library.
*/
#include <exec/types.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#define __NOLIBBASE__
#include <aros/kernel.h>
#include <proto/kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned checks, failures;

static void check(int ok, const char *label)
{
    ++checks;
    if (!ok)
        ++failures;
    printf("%s %s\n", ok ? "ok  " : "FAIL", label);
    fflush(stdout);
}

static int pixel(volatile UBYTE *framebuffer, ULONG pitch,
                 unsigned x, unsigned y, UBYTE red, UBYTE green, UBYTE blue)
{
    volatile UBYTE *p = framebuffer + (size_t)y * pitch + x * 4;

    if (p[0] == red && p[1] == green && p[2] == blue)
        return 1;
    printf("pixel(%u,%u)=%u,%u,%u expected=%u,%u,%u\n",
           x, y, p[0], p[1], p[2], red, green, blue);
    fflush(stdout);
    return 0;
}

static struct Screen *open_screen(ULONG id, ULONG depth, ULONG width, ULONG height)
{
    return OpenScreenTags(NULL, SA_DisplayID, id, SA_Width, width,
                          SA_Height, height, SA_Depth, depth,
                          SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
                          SA_Behind, TRUE, TAG_DONE);
}

int main(void)
{
    struct KernelBase *KernelBase = OpenResource((CONST_STRPTR)"kernel.resource");
    IPTR address = KernelBase ? (IPTR)KrnGetSystemAttr(KATTR_FrameBuffer) : 0;
    IPTR width = KernelBase ? KrnGetSystemAttr(KATTR_FrameBufferWidth) : 0;
    IPTR height = KernelBase ? KrnGetSystemAttr(KATTR_FrameBufferHeight) : 0;
    IPTR pitch = KernelBase ? KrnGetSystemAttr(KATTR_FrameBufferPitch) : 0;
    IPTR depth = KernelBase ? KrnGetSystemAttr(KATTR_FrameBufferDepth) : 0;
    ULONG id = 0xFFFFFFFFUL, indexed = 0xFFFFFFFFUL, truecolor = 0xFFFFFFFFUL;
    struct Screen *tc = NULL, *scr = NULL;
    volatile UBYTE *fb = (volatile UBYTE *)address;
    unsigned i;

    check(address && address != (IPTR)-1 && depth == 32 &&
          width >= 160 && width <= 16384 && height >= 64 && height <= 16384 &&
          pitch >= width * 4 && pitch <= 65536 &&
          address <= UINTPTR_MAX - height * pitch,
          "valid native RGBX framebuffer attributes");
    if (failures)
        goto done;
    for (i = 0; i < 256; ++i)
    {
        struct DimensionInfo dim;

        id = NextDisplayInfo(id);
        if (id == 0xFFFFFFFFUL)
            break;
        memset(&dim, 0, sizeof(dim));
        if (!GetDisplayInfoData(NULL, (UBYTE *)&dim, sizeof(dim), DTAG_DIMS, id))
            continue;
        if (dim.MaxDepth > 8 && truecolor == 0xFFFFFFFFUL)
            truecolor = id;
        if (dim.MaxDepth == 8 && indexed == 0xFFFFFFFFUL)
            indexed = id;
    }
    check(truecolor != 0xFFFFFFFFUL, "original truecolor mode remains available");
    check(indexed != 0xFFFFFFFFUL, "driver enumerates an eight-bit mode");
    if (truecolor == 0xFFFFFFFFUL || indexed == 0xFFFFFFFFUL)
        goto done;

    tc = open_screen(truecolor, 24, width, height);
    check(tc != NULL, "open original truecolor screen");
    if (!tc)
        goto done;
    check(GetBitMapAttr(tc->RastPort.BitMap, BMA_DEPTH) > 8,
          "truecolor bitmap keeps its storage depth");
    check(tc->ViewPort.ColorMap != NULL, "truecolor screen owns a color map");
    if (!tc->ViewPort.ColorMap)
        goto done;
    SetRGB32(&tc->ViewPort, 1, 0x44444444UL, 0x77777777UL, 0xAAAAAAAAUL);
    SetAPen(&tc->RastPort, 1);
    RectFill(&tc->RastPort, 32, 32, 143, 47);
    ScreenToFront(tc);
    Delay(5);
    check(pixel(fb, pitch, 40, 40, 0x44, 0x77, 0xAA),
          "original truecolor bytes reach the framebuffer");

    scr = open_screen(indexed, 8, width, height);
    check(scr != NULL, "open LUT8 screen behind truecolor screen");
    if (!scr)
        goto done;
    check(GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) == 8,
          "indexed bitmap has eight-bit storage depth");
    check(scr->ViewPort.ColorMap && scr->ViewPort.ColorMap->Count == 256,
          "indexed screen owns 256 palette entries");
    if (!scr->ViewPort.ColorMap || scr->ViewPort.ColorMap->Count != 256)
        goto done;
    SetRGB32(&scr->ViewPort, 1, 0xFFFFFFFFUL, 0, 0);
    SetRGB32(&scr->ViewPort, 2, 0, 0xFFFFFFFFUL, 0);
    SetRGB32(&scr->ViewPort, 3, 0, 0, 0xFFFFFFFFUL);
    SetRGB32(&scr->ViewPort, 255, 0x11111111UL, 0x99999999UL, 0xEEEEEEEEUL);
    SetAPen(&scr->RastPort, 1);
    RectFill(&scr->RastPort, 32, 32, 47, 47);
    SetAPen(&scr->RastPort, 2);
    RectFill(&scr->RastPort, 64, 32, 79, 47);
    SetAPen(&scr->RastPort, 3);
    RectFill(&scr->RastPort, 96, 32, 111, 47);
    SetAPen(&scr->RastPort, 255);
    RectFill(&scr->RastPort, 128, 32, 143, 47);
    check(ReadPixel(&scr->RastPort, 40, 40) == 1 &&
          ReadPixel(&scr->RastPort, 72, 40) == 2 &&
          ReadPixel(&scr->RastPort, 104, 40) == 3 &&
          ReadPixel(&scr->RastPort, 136, 40) == 255,
          "source pixels retain all tested palette indices");
    check(pixel(fb, pitch, 40, 40, 0x44, 0x77, 0xAA),
          "hidden indexed drawing and palette changes preserve the visible screen");
    ScreenToFront(scr);
    Delay(5);
    check(pixel(fb, pitch, 40, 40, 255, 0, 0), "pre-rendered red is visible");
    check(pixel(fb, pitch, 72, 40, 0, 255, 0), "pre-rendered green is visible");
    check(pixel(fb, pitch, 104, 40, 0, 0, 255), "pre-rendered blue is visible");
    check(pixel(fb, pitch, 136, 40, 0x11, 0x99, 0xEE), "palette entry 255 is visible");
    SetRGB32(&scr->ViewPort, 1, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0);
    SetRGB32(&scr->ViewPort, 255, 0, 0xFFFFFFFFUL, 0xFFFFFFFFUL);
    Delay(5);
    check(ReadPixel(&scr->RastPort, 40, 40) == 1 &&
          ReadPixel(&scr->RastPort, 136, 40) == 255,
          "live palette changes preserve the source indices");
    check(pixel(fb, pitch, 40, 40, 255, 255, 0), "entry 1 recolors without redrawing");
    check(pixel(fb, pitch, 136, 40, 0, 255, 255), "entry 255 recolors without redrawing");
    check(pixel(fb, pitch, 72, 40, 0, 255, 0) &&
          pixel(fb, pitch, 104, 40, 0, 0, 255), "other palette entries stay unchanged");
    check(CloseScreen(scr), "close indexed screen");
    scr = NULL;
    Delay(5);
    check(pixel(fb, pitch, 40, 40, 0x44, 0x77, 0xAA),
          "closing indexed screen restores the original truecolor pixels");
done:
    if (scr)
        check(CloseScreen(scr), "close indexed screen after a failure");
    if (tc)
        check(CloseScreen(tc), "close truecolor screen");
    printf("fbgfx_palette: %u checks, %u failed\n", checks, failures);
    fflush(stdout);
    return failures ? 10 : 0;
}
