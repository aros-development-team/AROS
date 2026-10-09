/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Exercise classic sprite allocation, DMA control words, data replacement
    and release. Leave the visible, changed and freed stages up for native
    frame/register capture so the actual hardware publication is checked too.
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/sprite.h>
#include <graphics/monitor.h>
#include <graphics/copper.h>
#include <exec/memory.h>
#include <intuition/preferences.h>

static ULONG failures;

static UWORD *dmastream(WORD channel)
{
    UWORD *cop = (UWORD *)GfxBase->copinit;
    ULONG ptr = 0, i;

    if (!cop)
        return NULL;
    for (i = 0; i < 46; i++, cop += 2)
    {
        if (cop[0] == 0x120 + channel * 4)
            ptr |= (ULONG)cop[1] << 16;
        if (cop[0] == 0x122 + channel * 4)
            ptr |= cop[1];
    }
    return (UWORD *)ptr;
}

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("CLASSIC SPRITES FAIL: %s\n", what);
        failures++;
    }
}

static void position(struct ViewPort *vp, struct SimpleSprite *s)
{
    struct View *view = GfxBase->ActiView;
    UWORD x = s->x + STANDARD_VIEW_X + (view ? view->DxOffset : 0);
    UWORD y = s->y + STANDARD_VIEW_Y + (view ? view->DyOffset : 0);
    UWORD stop = y + s->height;
    UWORD pos = (y << 8) | ((x >> 1) & 0xff);
    UWORD ctl = (stop << 8) | ((y & 0x100) >> 6) |
                ((stop & 0x100) >> 7) | (x & 1);

    check(s->posctldata[0] == pos, "DMA start coordinates");
    check((s->posctldata[1] & ~SPRITE_ATTACHED) == ctl, "DMA stop coordinates");
    check(GfxBase->SimpleSprites && GfxBase->SimpleSprites[s->num] == s,
          "native channel owns the current descriptor");
}

int main(void)
{
    struct Screen *screen = NULL;
    struct SimpleSprite left = {0}, right = {0}, bad = {0};
    UWORD *streams = NULL;
    UWORD *first, *second, *replacement;
    UBYTE reserved = GfxBase->SpriteReserved;
    WORD oldx, oldy;
    ULONG i;
    BOOL own2 = FALSE, own3 = FALSE;
    struct Preferences prefs;
    struct ExtSprite *cursor = NULL;
    struct Window *window = NULL;

    GetPrefs(&prefs, sizeof(prefs));
    check(prefs.ViewInitX + prefs.ViewXOffset == STANDARD_VIEW_X &&
          prefs.ViewInitY + prefs.ViewYOffset == STANDARD_VIEW_Y,
          "legacy preferences report the native sprite origin");
    GetDefPrefs(&prefs, sizeof(prefs));
    check(prefs.ViewInitX == STANDARD_VIEW_X && prefs.ViewInitY == STANDARD_VIEW_Y,
          "default preferences expose the native beam origin");

    check(GetSprite(&bad, -2) == -1 && GetSprite(&bad, 8) == -1,
          "invalid allocations rejected");
    FreeSprite(-1);
    check(GfxBase->SpriteReserved == reserved, "invalid operations preserve reservations");
    streams = AllocMem(4 * 40, MEMF_CHIP | MEMF_CLEAR);
    if (!streams)
    {
        check(FALSE, "allocate sprite streams");
        goto end;
    }
    first = streams;
    second = streams + 20;
    replacement = streams + 40;
    for (i = 0; i < 8; i++)
    {
        first[2 + i * 2] = 0xffff;             /* Sprite pen 1. */
        second[3 + i * 2] = 0xffff;            /* Sprite pen 2. */
        replacement[2 + i * 2] = 0xffff;
        replacement[3 + i * 2] = 0xffff;       /* Sprite pen 3. */
    }
    left.posctldata = first;
    left.height = 8;
    right.posctldata = second;
    right.height = 8;
    own2 = GetSprite(&left, 2) == 2;
    own3 = GetSprite(&right, 3) == 3;
    check(own2 && own3, "reserve distinct hardware channels");
    if (!own2 || !own3)
        goto end;
    check(GetSprite(&bad, 2) == -1, "occupied channel rejected");
    screen = OpenScreenTags(NULL, SA_Width, 320, SA_Height, 100,
        SA_Depth, 5, SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
        SA_ShowTitle, FALSE, TAG_DONE);
    if (!screen)
    {
        check(FALSE, "open sprite screen");
        goto end;
    }
    SetRGB4(&screen->ViewPort, 0, 0, 0, 0);
    SetRGB4(&screen->ViewPort, 21, 15, 0, 0);
    SetRGB4(&screen->ViewPort, 22, 0, 15, 0);
    SetRGB4(&screen->ViewPort, 23, 15, 15, 0);
    SetRGB4(&screen->ViewPort, 17, 0, 0, 15);
    MoveSprite(&screen->ViewPort, &left, 40, 20);
    MoveSprite(&screen->ViewPort, &right, 100, 40);
    position(&screen->ViewPort, &left);
    position(&screen->ViewPort, &right);
    check(first[2] == 0xffff && first[3] == 0, "DMA image remains caller-owned");
    WaitTOF();
    check(dmastream(2) == first && dmastream(3) == second,
          "copper publishes both caller-owned streams");
    {
        struct TagItem tags[] = {
            {SPRITEA_OldDataFormat, TRUE}, {SPRITEA_Width, 16},
            {SPRITEA_OutputHeight, 8}, {TAG_DONE, 0}
        };
        cursor = AllocSpriteDataA((struct BitMap *)first, tags);
        check(cursor != NULL, "allocate HIDD cursor alongside classic sprites");
        if (cursor)
        {
            UWORD *data;
            UWORD expected = ((60 + STANDARD_VIEW_Y) << 8) |
                             (((160 + STANDARD_VIEW_X) >> 1) & 0xff);

            check(ChangeExtSpriteA(&screen->ViewPort, cursor, cursor, NULL) != 0,
                  "install converted cursor shape");
            MoveSprite(&screen->ViewPort, &cursor->es_SimpleSprite, 160, 60);
            WaitTOF();
            data = dmastream(0);
            check(data && data[0] == expected,
                  "converted cursor still reaches its display position");
        }
    }
    bug("CLASSIC SPRITES VISIBLE: channel 2 stream %p, channel 3 stream %p\n", first, second);
    Delay(100);

    /* SetPointer has a different lifetime contract from AllocSpriteData:
     * a legacy Chip RAM pointer remains live until it is cleared. */
    window = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)screen,
        WA_Left, 0, WA_Top, 0, WA_Width, 320, WA_Height, 100,
        WA_Borderless, TRUE, WA_Activate, TRUE, TAG_DONE);
    check(window != NULL, "open legacy pointer window");
    if (window)
    {
        UWORD *pointer = streams + 60;
        UWORD pos = (80 << 8) | (250 >> 1);

        for (i = 0; i < 8; i++)
            pointer[2 + i * 2] = 0xffff;
        SetPointer(window, pointer, 8, 16, 0, 0);
        Delay(3);
        check(dmastream(0) == pointer, "legacy Chip RAM cursor uses live DMA data");
        pointer[0] = pos;
        pointer[1] = 88 << 8;
        pointer[2] = 0;
        pointer[3] = 0xffff;
        WaitTOF();
        WaitTOF();
        check(pointer[0] == pos && dmastream(0) == pointer,
              "application cursor updates survive vertical blank");
        ClearPointer(window);
        Delay(3);
        check(dmastream(0) != pointer, "clear releases the borrowed cursor data");
        CloseWindow(window);
        window = NULL;
    }

    /* ChangeSprite must not add a ViewPort offset twice. */
    oldx = screen->ViewPort.DxOffset;
    oldy = screen->ViewPort.DyOffset;
    screen->ViewPort.DxOffset = 10;
    screen->ViewPort.DyOffset = 12;
    MoveSprite(&screen->ViewPort, &left, 40, 20);
    ChangeSprite(&screen->ViewPort, &left, replacement);
    check(left.x == 50 && left.y == 32, "replacement preserves logical position");
    position(&screen->ViewPort, &left);
    WaitTOF();
    check(dmastream(2) == replacement, "replacement reaches the hardware stream");
    screen->ViewPort.DxOffset = oldx;
    screen->ViewPort.DyOffset = oldy;
    MoveSprite(&screen->ViewPort, &left, 40, 20);
    bug("CLASSIC SPRITES CHANGED: channel 2 stream %p\n", replacement);
    Delay(50);
    FreeSprite(2);
    own2 = FALSE;
    check(!(GfxBase->SpriteReserved & 4) &&
          GfxBase->SimpleSprites && !GfxBase->SimpleSprites[2], "free unpublishes channel");
    bug("CLASSIC SPRITES FREED: channel 2\n");
    WaitTOF();
    check(dmastream(2) != replacement && dmastream(3) == second,
          "release hides one channel while preserving the other");
    Delay(50);
    own2 = GetSprite(&left, 2) == 2;
    check(own2, "freed channel can be reused");
end:
    if (window)
        CloseWindow(window);
    if (own2)
        FreeSprite(2);
    if (own3)
        FreeSprite(3);
    WaitTOF();
    if (screen)
        CloseScreen(screen);
    if (cursor)
        FreeSpriteData(cursor);
    if (streams)
        FreeMem(streams, 4 * 40);
    bug("CLASSIC SPRITES TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
