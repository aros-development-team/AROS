/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/layers.h>
#include <graphics/modeid.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <utility/hooks.h>
#include <string.h>

static ULONG checks, failures;

static AROS_UFH3(ULONG, no_backfill,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT
    return 0;
    AROS_USERFUNC_EXIT
}

static struct Hook backfill_hook = {
    { NULL, NULL }, (HOOKFUNC)no_backfill, NULL, NULL
};

static void check(BOOL okay, CONST_STRPTR text)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("SCREEN BITMAP FAIL: %s\n", text);
    }
}

static void layout(struct BitMap *actual, struct BitMap *expected)
{
    ULONG p;

    check(actual->BytesPerRow == expected->BytesPerRow,
          "screen preserves the bitmap row stride");
    check((actual->Flags & BMF_INTERLEAVED) ==
          (expected->Flags & BMF_INTERLEAVED),
          "screen preserves bitmap interleaving");
    check(actual->Rows == expected->Rows && actual->Depth == expected->Depth,
          "screen preserves bitmap dimensions and depth");
    for (p = 0; p < expected->Depth; p++)
        check(actual->Planes[p] == expected->Planes[p],
              "screen preserves plane addresses");
}

static void planar(BOOL interleaved, ULONG backfill)
{
    struct BitMap bitmap, original;
    struct Screen *screen;
    struct ScreenBuffer *buffer;
    UBYTE *pixels = AllocMem(40 * 64 * 8, MEMF_CHIP | MEMF_CLEAR);
    ULONG p;
    ULONG error = 0;

    bug("SCREEN BITMAP CASE: %s, backfill %lu\n",
        interleaved ? "interleaved" : "separate", backfill);
    check(pixels != NULL, "allocate planar pixel storage");
    if (!pixels)
        return;
    InitBitMap(&bitmap, 8, 320, 64);
    if (interleaved)
    {
        bitmap.Flags |= BMF_INTERLEAVED;
        bitmap.BytesPerRow *= bitmap.Depth;
    }
    for (p = 0; p < 8; p++)
        bitmap.Planes[p] = pixels + p * (interleaved ? 40 : 40 * 64);
    if (backfill)
        memset(pixels, 0x5a, 40 * 64 * 8);
    original = bitmap;
    screen = OpenScreenTags(NULL, SA_Width, 320, SA_Height, 64,
        SA_Depth, 8, SA_Type, CUSTOMSCREEN, SA_Quiet, TRUE,
        SA_DisplayID, PAL_MONITOR_ID | LORES_KEY, SA_ErrorCode, (IPTR)&error,
        SA_ShowTitle, FALSE, SA_BitMap, (IPTR)&bitmap,
        SA_BackFill, backfill == 2 ? (IPTR)&backfill_hook :
            (IPTR)(backfill ? LAYERS_NOBACKFILL : LAYERS_BACKFILL), TAG_DONE);
    check(screen != NULL, "open caller-owned planar screen");
    if (!screen)
        bug("SCREEN BITMAP: OpenScreen error %lu\n", error);
    if (screen)
    {
        bug("SCREEN BITMAP CASE: screen opened\n");
        layout(screen->RastPort.BitMap, &original);
        layout(screen->ViewPort.RasInfo->BitMap, &original);
        check(memcmp(&bitmap, &original, sizeof(bitmap)) == 0,
              "opening a screen preserves the caller's header");
        if (backfill)
        {
            ULONG byte;
            BOOL intact = TRUE;

            for (byte = 0; byte < 40 * 64 * 8; byte++)
                if (pixels[byte] != 0x5a)
                    intact = FALSE;
            check(intact, "no-backfill screen preserves preloaded pixels");
        }
        if (screen->RastPort.BitMap->BytesPerRow == original.BytesPerRow)
        {
            SetAPen(&screen->RastPort, 1);
            WritePixel(&screen->RastPort, 8, 20);
            WaitBlit();
            check((pixels[20 * original.BytesPerRow + 1] & 0x80) != 0,
                  "drawing reaches the correct physical bitmap row");
        }
        buffer = AllocScreenBuffer(screen, &bitmap, 0);
        check(buffer != NULL, "allocate caller-owned screen buffer");
        if (buffer)
        {
            check(ChangeScreenBuffer(screen, buffer), "select screen buffer");
            WaitTOF();
            WaitTOF();
            layout(&screen->BitMap_OBSOLETE, &original);
            layout(screen->ViewPort.RasInfo->BitMap, &original);
            check(memcmp(&bitmap, &original, sizeof(bitmap)) == 0,
                  "changing buffers preserves the caller's header");
            FreeScreenBuffer(screen, buffer);
        }
        CloseScreen(screen);
        bug("SCREEN BITMAP CASE: screen closed\n");
    }
    FreeMem(pixels, 40 * 64 * 8);
}

int main(void)
{
    planar(FALSE, 0);
    planar(TRUE, 0);
    planar(TRUE, 1);
    planar(TRUE, 2);
    bug("SCREEN BITMAP TEST: %lu/%lu pass, %lu failures\n",
        checks - failures, checks, failures);
    return failures ? 20 : 0;
}
