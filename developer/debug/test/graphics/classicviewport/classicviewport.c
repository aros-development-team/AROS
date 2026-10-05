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
    struct BitMap bitmap;
    struct BitMap drawing_before;
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
    rebuild(screen, &bitmap);
    bitmap.Depth = 5;
    rebuild(screen, &bitmap);
    bitmap.Depth = 3;
    rebuild(screen, &bitmap);
    bitmap.Depth = 6;
    rebuild(screen, &bitmap);
    bitmap.Depth = 3;
    bitmap.Planes[0] = planes[6];
    rebuild(screen, &bitmap);
    bitmap.Planes[0] = planes[0];
    rebuild(screen, &bitmap);

    /* The old Screen bitmap is a separate public mirror. Liberation
     * changes this field rather than RastPort.BitMap. */
    drawing_before = bitmap;
    screen->BitMap_OBSOLETE.Depth = 5;
    screen->BitMap_OBSOLETE.Planes[0] = planes[6];
    rebuild(screen, &screen->BitMap_OBSOLETE);
    check(screen->ViewPort.RasInfo->BitMap->Depth == 5 &&
          screen->ViewPort.RasInfo->BitMap->Planes[0] == planes[6],
          "legacy screen bitmap changes reach the display bitmap");
    check(memcmp(&bitmap, &drawing_before, sizeof(bitmap)) == 0,
          "legacy display changes preserve the caller's drawing bitmap");
    screen->BitMap_OBSOLETE.Depth = 3;
    rebuild(screen, &screen->BitMap_OBSOLETE);
    check(screen->ViewPort.RasInfo->BitMap->Depth == 3, "legacy bitmap can shrink again");
    check(memcmp(&bitmap, &drawing_before, sizeof(bitmap)) == 0,
          "repeated legacy changes preserve distinct double-buffer planes");

    /* A later modern bitmap change must not be overwritten by an
     * unchanged legacy mirror. */
    screen->ViewPort.RasInfo->BitMap = &bitmap;
    bitmap.Depth = 6;
    rebuild(screen, &bitmap);
    check(bitmap.Depth == 6, "unchanged legacy mirror preserves modern edits");
    bitmap.Depth = 3;
    rebuild(screen, &bitmap);
    bug("CLASSIC VIEWPORT READY: depth %lu, retained higher planes\n", (unsigned long)bitmap.Depth);
    Delay(250);
end:
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
