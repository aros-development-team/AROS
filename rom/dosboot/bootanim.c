/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

/*
 * This animation code draws a picture centered on the screen
 * and flashes a specified region once per second.
 */

#include <aros/debug.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "bootanim.h"

extern struct AnimData *banm_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase);
extern void banm_Dispose(struct DOSBootBase *DOSBootBase);
extern void banm_ShowFrame(struct Screen *scr, BOOL init, struct DOSBootBase *DOSBootBase);

// core flags...
#define STATEF_POINTERHIDE              (1 << 0)

void WriteChunkRegion(struct DOSBootBase *DOSBootBase,
                                    struct RastPort *rp, UWORD x, UWORD y, UWORD width,
                                    UBYTE *data, UWORD regx, UWORD regy, UWORD regwidth, UWORD regheight)
{
    WriteChunkyPixels(rp, x + regx, y + regy,
                                    x + regx + regwidth - 1, y + regy + regheight - 1,
                                    data + regx + (regy * width), width);
}

APTR anim_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad;

    D(bug("Screen: %dx%dx%d\n", scr->Width, scr->Height, scr->RastPort.BitMap->Depth));

    ad = banm_Init(scr, DOSBootBase);
    if (ad)
    {
        D(bug("[dosboot] %s: ad @ 0x%p\n", __func__, ad);)
        DOSBootBase->blank_pointer = AllocMem(6*2, MEMF_CHIP | MEMF_CLEAR);
        SetPointer(DOSBootBase->bm_Window, DOSBootBase->blank_pointer,
                   1, 16, 0, 0);
        ad->ad_State |= STATEF_POINTERHIDE;
        SetAPen(&scr->RastPort, 0);
        RectFill(&scr->RastPort, 0, 0, scr->Width, scr->Height);
        DOSBootBase->animData = ad;
        DOSBootBase->delayTicks = 25;   /* We run at 2 frames per second */

        banm_ShowFrame(scr, TRUE, DOSBootBase);
        return ad;
    }

    return NULL;
}

void anim_Animate(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;
    if (ad)
    {
        ad->frame = ad->tick++ % ad->framecnt;
        D(bug("[dosboot] %s: frame %u, tick %u\n", __func__, ad->frame, ad->tick);)
        banm_ShowFrame(scr, FALSE, DOSBootBase);
    }
}

void anim_Stop(struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;
    if (ad)
    {
        if (ad->ad_State & STATEF_POINTERHIDE)
        {
            FreeMem(DOSBootBase->blank_pointer, 6*2);
            DOSBootBase->blank_pointer = NULL;
        }
        banm_Dispose(DOSBootBase);
    }
}
