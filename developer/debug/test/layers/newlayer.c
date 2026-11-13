/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: A smart refresh layer created behind a window with
          CreateBehindHookLayer(): AmigaOS (Kickstart 3.1) runs the backfill
          hook for its hidden part into the backing bitmap, so the covered
          part shows the backfill once uncovered.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/cybergraphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <graphics/rpattr.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <aros/asmcall.h>
#include <stdio.h>

#define BACKCOL 0x2040C0

struct layerhookmsg { struct Layer *l; WORD MinX, MinY, MaxX, MaxY; LONG OffsetX, OffsetY; };
static LONG hookcalls, hookhidden;
static struct BitMap *screenbm;

AROS_UFH3(void, BackFillFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct RastPort *, rp, A2),
    AROS_UFHA(struct layerhookmsg *, msg, A1))
{
    AROS_USERFUNC_INIT
    struct RastPort tmp = *rp;
    tmp.Layer = NULL;
    hookcalls++;
    if (rp->BitMap != screenbm) hookhidden++;
    SetRPAttrs(&tmp, RPTAG_PenMode, FALSE, RPTAG_FgColor, BACKCOL, TAG_DONE);
    RectFill(&tmp, msg->MinX, msg->MinY, msg->MaxX, msg->MaxY);
    AROS_USERFUNC_EXIT
}

int main(void)
{
    struct Screen *scr;
    struct Window *w2 = NULL;
    struct Layer *L = NULL;
    struct Hook hook;
    ULONG vis, cov, after;
    LONG rc = RETURN_FAIL;

    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(BackFillFunc);
    hook.h_Data = NULL;
    scr = LockPubScreen(NULL);
    if (!scr) return RETURN_FAIL;
    screenbm = scr->RastPort.BitMap;

    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!w2) goto done;
    Delay(10);

    L = CreateBehindHookLayer(&scr->LayerInfo, scr->RastPort.BitMap, 60, 60, 379, 259, LAYERSMART, &hook, NULL);
    printf("newlayer: layer %p; backfill hook calls at creation %ld, %ld into hidden bitmaps\n", L, (long)hookcalls, (long)hookhidden);
    if (!L) goto done;
    vis = ReadRGBPixel(L->rp, 100, 100) & 0xFFFFFF;
    cov = ReadRGBPixel(L->rp, 250, 150) & 0xFFFFFF;
    CloseWindow(w2); w2 = NULL;
    Delay(10);
    after = ReadRGBPixel(L->rp, 250, 150) & 0xFFFFFF;
    printf("  visible part %06lx, covered part through the rastport %06lx, same spot after uncovering %06lx (want %06x)\n",
           (unsigned long)vis, (unsigned long)cov, (unsigned long)after, BACKCOL);
    rc = (vis == BACKCOL && cov == BACKCOL && after == BACKCOL && hookhidden > 0) ? RETURN_OK : RETURN_WARN;
    printf("RESULT: %s\n", rc == RETURN_OK ? "PASS" : "FAIL");
done:
    if (L) DeleteLayer(0, L);
    if (w2) CloseWindow(w2);
    UnlockPubScreen(NULL, scr);
    return rc;
}
