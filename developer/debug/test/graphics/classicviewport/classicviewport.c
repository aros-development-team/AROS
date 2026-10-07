/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Rebuild a classic BitMap whose address stays fixed while its depth,
    dimensions and plane addresses change. Native copper checks verify
    the display layout, rather than just the caller's modified structure.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/screens.h>
#include <graphics/copper.h>
#include <string.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("CLASSIC VIEWPORT FAIL: %s\n", what);
        failures++;
    }
}

#ifdef __mc68000__
static LONG coppermove(struct ViewPort *vp, UWORD reg)
{
    struct CopList *cl = vp->DspIns;
    UWORD *p;
    ULONG i;

    if (!cl || !cl->CopLStart)
        return -1;
    p = cl->CopLStart;
    for (i = 0; i < (ULONG)cl->MaxCount; i++, p += 2)
    {
        if (p[0] == 0xffff && p[1] == 0xfffe)
            break;
        if (p[0] == reg)
            return p[1];
    }
    return -1;
}
#endif

static void rebuild(struct Screen *screen, struct BitMap *bitmap)
{
    check(MakeScreen(screen) == 0 && RethinkDisplay() == 0, "rebuild classic bitmap");
#ifdef __mc68000__
    {
        LONG con0 = coppermove(&screen->ViewPort, 0x100);
        LONG hi = coppermove(&screen->ViewPort, 0xe0);
        LONG lo = coppermove(&screen->ViewPort, 0xe2);

        check(con0 >= 0 && ((con0 >> 12) & 7) == bitmap->Depth,
              "copper uses current bitmap depth");
        check(hi >= 0 && lo >= 0 &&
              (((ULONG)hi << 16) | lo) == (ULONG)bitmap->Planes[0],
              "copper uses current plane address");
    }
#endif
}

int main(void)
{
    struct Screen *screen = NULL;
    struct Screen *ordinary = NULL;
    struct ScreenBuffer *original_buffer = NULL, *other_buffer = NULL;
    struct BitMap bitmap;
    struct BitMap other;
    struct BitMap drawing_before;
    struct BitMap *display;
    PLANEPTR planes[7] = {0};
    ULONG i;

    InitBitMap(&bitmap, 6, 320, 100);
    for (i = 0; i < 7; i++)
    {
        planes[i] = AllocRaster(320, 100);
        if (!planes[i])
        {
            check(FALSE, "allocate test planes");
            goto end;
        }
        memset(planes[i], i >= 3 ? 0xff : 0, RASSIZE(320, 100));
        if (i < 6)
            bitmap.Planes[i] = planes[i];
    }
    screen = OpenScreenTags(NULL, SA_Width, 320, SA_Height, 100,
        SA_Depth, 6, SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
        SA_ShowTitle, FALSE, SA_BitMap, (IPTR)&bitmap, TAG_DONE);
    if (!screen)
    {
        check(FALSE, "open test screen");
        goto end;
    }
    drawing_before = bitmap;
    display = &screen->BitMap_OBSOLETE;
    check(screen->ViewPort.RasInfo->BitMap == display,
          "raw custom screen displays its embedded bitmap header");
    check(screen->RastPort.BitMap == display,
          "raw custom drawing uses the embedded bitmap header");
    rebuild(screen, display);

    /* Legacy drawing code may replace plane pointers through RastPort.
     * That must change the displayed bitmap without changing the caller's
     * original header, as on Kickstart. */
    screen->RastPort.BitMap->Planes[0] = planes[6];
    rebuild(screen, screen->RastPort.BitMap);
    check(display->Planes[0] == planes[6],
          "drawing plane changes reach the display header");
    screen->RastPort.BitMap->Planes[0] = planes[0];
    rebuild(screen, display);
    display->Depth = 5;
    rebuild(screen, display);
    display->Depth = 3;
    rebuild(screen, display);
    display->Depth = 6;
    rebuild(screen, display);
    display->Depth = 3;
    display->Planes[0] = planes[6];
    rebuild(screen, display);
    display->Planes[0] = planes[0];
    rebuild(screen, display);
    check(memcmp(&bitmap, &drawing_before, sizeof(bitmap)) == 0,
          "legacy display changes preserve the caller's original bitmap");

    /* Explicit pointer swaps select the display header. Later embedded
     * edits must not override that choice or alias the drawing buffers. */
    screen->ViewPort.RasInfo->BitMap = &bitmap;
    bitmap.Depth = 6;
    rebuild(screen, &bitmap);
    other = bitmap;
    other.Planes[0] = planes[6];
    screen->ViewPort.RasInfo->BitMap = &other;
    rebuild(screen, &other);
    display->Depth = 5;
    rebuild(screen, &other);
    check(screen->ViewPort.RasInfo->BitMap == &other,
          "embedded edits do not override an explicit display pointer");
    check(bitmap.Planes[0] == planes[0] && other.Planes[0] == planes[6],
          "drawing buffers keep distinct plane addresses");
    screen->ViewPort.RasInfo->BitMap = &bitmap;
    bitmap.Depth = 3;
    rebuild(screen, &bitmap);

    other.Depth = 3;
    original_buffer = AllocScreenBuffer(screen, &bitmap, 0);
    other_buffer = AllocScreenBuffer(screen, &other, 0);
    check(original_buffer && other_buffer, "allocate caller-owned screen buffers");
    if (original_buffer && other_buffer)
    {
        check(ChangeScreenBuffer(screen, other_buffer), "select alternate screen buffer");
        rebuild(screen, &other);
        check(screen->ViewPort.RasInfo->BitMap == &other &&
              screen->RastPort.BitMap == &other,
              "screen-buffer swap selects both display and drawing header");
        check(ChangeScreenBuffer(screen, original_buffer), "restore original screen buffer");
        rebuild(screen, &bitmap);
        check(screen->ViewPort.RasInfo->BitMap == &bitmap &&
              screen->RastPort.BitMap == &bitmap,
              "screen-buffer restore retains the selected header");
    }

    ordinary = OpenScreenTags(NULL, SA_Width, 320, SA_Height, 100,
        SA_Depth, 3, SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
        SA_ShowTitle, FALSE, TAG_DONE);
    check(ordinary != NULL, "open ordinary managed screen");
    if (ordinary)
    {
        check(ordinary->ViewPort.RasInfo->BitMap == ordinary->RastPort.BitMap,
              "ordinary managed screens retain their original display header");
        check(MakeScreen(ordinary) == 0 && RethinkDisplay() == 0,
              "rebuild ordinary managed screen");
        CloseScreen(ordinary);
        ordinary = NULL;
    }
    bug("CLASSIC VIEWPORT READY: depth %lu, retained higher planes\n", (unsigned long)bitmap.Depth);
    Delay(250);
end:
    if (ordinary) CloseScreen(ordinary);
    if (other_buffer) FreeScreenBuffer(screen, other_buffer);
    if (original_buffer) FreeScreenBuffer(screen, original_buffer);
    if (screen)
    {
        CloseScreen(screen);
    }
    for (i = 0; i < 7; i++)
        if (planes[i])
            FreeRaster(planes[i], 320, 100);
    bug("CLASSIC VIEWPORT TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
