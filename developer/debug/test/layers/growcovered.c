/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: A smart refresh window enlarged while another window covers part
          of the new area. AmigaOS (checked on Kickstart 3.1) runs the
          layer's backfill hook into the backing bitmap for the covered
          part, so after uncovering the whole new area shows the backfill
          and the old covered content is intact.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <intuition/intuition.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/clip.h>
#include <graphics/layers.h>
#include <aros/asmcall.h>
#include <stdio.h>

#define BACKCOL 0x2040C0

struct layerhookmsg { struct Layer *l; WORD MinX, MinY, MaxX, MaxY; LONG OffsetX, OffsetY; };
static LONG hookcalls, hookhidden;

AROS_UFH3(void, BackFillFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct RastPort *, rp, A2),
    AROS_UFHA(struct layerhookmsg *, msg, A1))
{
    AROS_USERFUNC_INIT
    struct RastPort tmp = *rp;
    tmp.Layer = NULL;
    hookcalls++;
    if (msg->l && rp->BitMap != msg->l->rp->BitMap) hookhidden++;
    SetRPAttrs(&tmp, RPTAG_PenMode, FALSE, RPTAG_FgColor, BACKCOL, TAG_DONE);
    RectFill(&tmp, msg->MinX, msg->MinY, msg->MaxX, msg->MaxY);
    AROS_USERFUNC_EXIT
}

int main(void)
{
    struct Window *w1, *w2;
    struct Hook hook;
    LONG x, y, bad = 0, checked = 0, rc = RETURN_FAIL;
    ULONG vis;

    hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(BackFillFunc);
    hook.h_Data = NULL;

    w1 = OpenWindowTags(NULL, WA_Left, 60, WA_Top, 60, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"growcovered", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_SizeGadget, TRUE,
            WA_MaxWidth, 600, WA_MaxHeight, 400, WA_BackFill, (IPTR)&hook, WA_Activate, FALSE, TAG_DONE);
    w2 = OpenWindowTags(NULL, WA_Left, 220, WA_Top, 180, WA_Width, 320, WA_Height, 200,
            WA_Title, (IPTR)"cover", WA_SmartRefresh, TRUE, WA_DragBar, TRUE, WA_Activate, FALSE, TAG_DONE);
    if (!w1 || !w2) goto done;
    Delay(10);
    if (GetBitMapAttr(w1->RPort->BitMap, BMA_DEPTH) < 15) { printf("needs a true colour screen\n"); goto done; }

    /* give the window a non-zero background so leftovers would show */
    SetRPAttrs(w1->RPort, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0xFF8040, TAG_DONE);
    RectFill(w1->RPort, w1->BorderLeft, w1->BorderTop, w1->Width - w1->BorderRight - 1, w1->Height - w1->BorderBottom - 1);

    hookcalls = hookhidden = 0;
    SizeWindow(w1, 60, 0);
    Delay(10);
    printf("growcovered: window now %dx%d; backfill hook calls during grow %ld, %ld into hidden bitmaps\n",
           w1->Width, w1->Height, (long)hookcalls, (long)hookhidden);
    printf("  before uncovering, through the rastport: new area visible part (330,70) = %06lx, covered part (330,150) = %06lx, old inner (100,100) = %06lx\n",
           (unsigned long)(ReadRGBPixel(w1->RPort, 330, 70) & 0xFFFFFF),
           (unsigned long)(ReadRGBPixel(w1->RPort, 330, 150) & 0xFFFFFF),
           (unsigned long)(ReadRGBPixel(w1->RPort, 100, 100) & 0xFFFFFF));
    CloseWindow(w2); w2 = NULL;
    Delay(10);
    printf("  after uncovering: (330,70) = %06lx, (330,150) = %06lx, old inner (100,100) = %06lx, old covered inner (250,150) = %06lx\n",
           (unsigned long)(ReadRGBPixel(w1->RPort, 330, 70) & 0xFFFFFF),
           (unsigned long)(ReadRGBPixel(w1->RPort, 330, 150) & 0xFFFFFF),
           (unsigned long)(ReadRGBPixel(w1->RPort, 100, 100) & 0xFFFFFF),
           (unsigned long)(ReadRGBPixel(w1->RPort, 250, 150) & 0xFFFFFF));

    /* AmigaOS: the part of the new area that was under the cover window
       (x 320.., y >= 120) shows the backfill like the visible part, and the
       old covered content is intact */
    vis = ReadRGBPixel(w1->RPort, 330, 70) & 0xFFFFFF;
    for (y = 125; y <= w1->Height - w1->BorderBottom - 1; y++)
        for (x = 320; x <= w1->Width - w1->BorderRight - 1; x++)
        {
            ULONG v = ReadRGBPixel(w1->RPort, x, y) & 0xFFFFFF;
            checked++;
            if (v != BACKCOL) bad++;
        }
    printf("  new area after uncovering: visible part %06lx, %ld of %ld covered-part pixels are not the backfill colour %06x\n",
           (unsigned long)vis, (long)bad, (long)checked, BACKCOL);
    {
        ULONG oldcov = ReadRGBPixel(w1->RPort, 250, 150) & 0xFFFFFF;
        BOOL ok = (bad == 0 && vis == BACKCOL && oldcov == 0xFF8040 && hookhidden > 0);
        printf("  old covered content %06lx (want ff8040)\n", (unsigned long)oldcov);
        printf("RESULT: %s\n", ok ? "PASS" : "FAIL");
        rc = ok ? RETURN_OK : RETURN_WARN;
    }
done:
    if (w2) CloseWindow(w2);
    if (w1) CloseWindow(w1);
    return rc;
}
