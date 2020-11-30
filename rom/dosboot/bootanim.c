/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This animation code draws a picture centered on the screen
 * and flashes a specified region once per second.
 */

#define __OOP_NOLIBBASE__
#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include <aros/debug.h>
#include <intuition/screens.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <hidd/gfx.h>

#include "bootanim.h"

extern struct AnimData *banm_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase);
extern void banm_Dispose(struct DOSBootBase *DOSBootBase);
extern void banm_ShowFrame(struct Screen *scr, BOOL init, struct DOSBootBase *DOSBootBase);

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_Hidd_Gfx,
    NULL
};
#endif

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
        if ((ad->OOPBase = OpenLibrary("oop.library",0)) != NULL)
        {
           struct OOP_ABDescr attrbases[] = 
            {
                { IID_Hidd_BitMap,  &ad->bitMapAttrBase     },
                { NULL,             NULL                    }
            };
            #define OOPBase ad->OOPBase
            D(bug("[dosboot] %s: OOPBase @ 0x%p\n", __func__, OOPBase);)
#if defined(__OOP_NOATTRBASES__)
            if (OOP_ObtainAttrBases(attrbases))
            {
#endif
#if defined(__OOP_NOMETHODBASES__)
                if (OOP_ObtainMethodBasesArray(&ad->gfxMethodBase, methBaseIDs))
                {
#endif
                    #undef HiddBitMapAttrBase
                    #define HiddBitMapAttrBase (ad->bitMapAttrBase)
                    D(bug("[dosboot] %s: Bitmap hidd @ 0x%p\n", __func__, HIDD_BM_OBJ(scr->RastPort.BitMap));)
                    OOP_GetAttr(HIDD_BM_OBJ(scr->RastPort.BitMap), aHidd_BitMap_GfxHidd, (IPTR *)&ad->gfxhidd);
                    if (ad->gfxhidd){
                        D(bug("[dosboot] %s: Gfx hidd @ 0x%p\n", __func__, ad->gfxhidd);)
                        struct pHidd_Gfx_SetCursorVisible p;
#if defined(__OOP_NOMETHODBASES__)
                        p.mID = ad->gfxMethodBase;
#endif
                        p.mID += moHidd_Gfx_SetCursorVisible;
                        p.visible = FALSE;
                        (VOID)OOP_DoMethod(ad->gfxhidd, &p.mID);
                    }
#if defined(__OOP_NOMETHODBASES__)
                }
#endif
#if defined(__OOP_NOATTRBASES__)
            }
#endif
        }

        SetAPen(&scr->RastPort, 0);
        RectFill(&scr->RastPort, 0, 0, scr->Width, scr->Height);
        DOSBootBase->animData = ad;
        DOSBootBase->delayTicks = 25;	/* We run at 2 frames per second */

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
#if (0)    
        struct pHidd_Gfx_SetCursorVisible p;
        p.mID = ad->gfxMethodBase + moHidd_Gfx_SetCursorVisible;
        p.visible = TRUE;
        (VOID)OOP_DoMethod(ad->gfxhidd, &p.mID);
    
#endif
        banm_Dispose(DOSBootBase);
    }
}
