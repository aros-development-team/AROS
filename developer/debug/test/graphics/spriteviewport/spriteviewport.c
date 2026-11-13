/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/sprite.h>
#include <graphics/monitor.h>
#include <exec/memory.h>

static ULONG failures;

static void check(BOOL okay, CONST_STRPTR what)
{
    if (!okay)
    {
        bug("SPRITE VIEWPORT FAIL: %s\n", what);
        failures++;
    }
}

static UWORD *published_stream(void)
{
    UWORD *cop = (UWORD *)GfxBase->copinit;
    ULONG ptr = 0, i;

    if (!cop)
        return NULL;
    for (i = 0; i < 46; i++, cop += 2)
    {
        if (cop[0] == 0x128)
            ptr |= (ULONG)cop[1] << 16;
        if (cop[0] == 0x12a)
            ptr |= cop[1];
    }
    return (UWORD *)ptr;
}

int main(void)
{
    struct ViewPort vp;
    struct RasInfo ri = {0};
    struct BitMap bm;
    struct SimpleSprite sprite = {0};
    UWORD *stream = NULL;
    BOOL owned = FALSE;
    UWORD x, y, pos, ctl;
    struct View *view;

    InitVPort(&vp);
    InitBitMap(&bm, 1, 320, 200);
    ri.BitMap = &bm;
    vp.RasInfo = &ri;
    vp.DxOffset = 10;
    vp.DyOffset = 12;
    vp.ColorMap = GetColorMap(4);
    stream = AllocMem(80, MEMF_CHIP | MEMF_CLEAR);
    check(vp.ColorMap && stream, "allocate viewport and sprite data");
    if (!vp.ColorMap || !stream)
        goto end;
    sprite.height = 8;
    sprite.posctldata = stream;
    owned = GetSprite(&sprite, 2) == 2;
    check(owned, "reserve native sprite channel");
    if (!owned)
        goto end;
    stream[2] = 0xffff;

    /* No MakeVPort or MrgCop: games position sprites during setup. */
    bug("SPRITE VIEWPORT: positioning before MakeVPort\n");
    MoveSprite(&vp, &sprite, 40, 20);
    check(sprite.x == 50 && sprite.y == 32, "apply viewport offsets once");
    view = GfxBase->ActiView;
    x = 50 + STANDARD_VIEW_X + (view ? view->DxOffset : 0);
    y = 32 + STANDARD_VIEW_Y + (view ? view->DyOffset : 0);
    pos = (y << 8) | ((x >> 1) & 0xff);
    ctl = ((y + 8) << 8) | ((y & 0x100) >> 6) |
          (((y + 8) & 0x100) >> 7) | (x & 1);
    check(stream[0] == pos && stream[1] == ctl,
          "unbuilt viewport updates the live DMA control words");
    check(stream[2] == 0xffff, "positioning preserves sprite pixels");
    WaitTOF();
    WaitTOF();
    check(published_stream() == stream, "native copper publishes the live stream");

    ChangeSprite(&vp, &sprite, stream + 20);
    check(sprite.x == 50 && sprite.y == 32, "ChangeSprite preserves position");
    check(stream[20] == pos && stream[21] == ctl,
          "ChangeSprite prepares the replacement stream before MakeVPort");
    WaitTOF();
    WaitTOF();
    check(published_stream() == stream + 20,
          "native copper publishes the replacement stream");
    stream[20] = stream[21] = 0;
    MoveSprite(NULL, &sprite, 50, 32);
    check(stream[20] == pos && stream[21] == ctl,
          "NULL viewport retains native sprite positioning");
end:
    if (owned)
        FreeSprite(2);
    WaitTOF();
    if (stream)
        FreeMem(stream, 80);
    if (vp.ColorMap)
        FreeColorMap(vp.ColorMap);
    bug("SPRITE VIEWPORT TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
