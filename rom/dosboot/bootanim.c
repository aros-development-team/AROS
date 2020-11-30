/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This animation code draws a picture centered on the screen
 * and flashes a specified region once per second.
 */

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

// core flags...
#define STATEF_OOPATTRIBS               (1 << 1)
#define STATEF_POINTERHIDE              (1 << 2)

#if defined(__OOP_NOATTRBASES__)
static CONST_STRPTR const attrbaseIDs[] = 
{
    IID_Hidd_BitMap,
    NULL
};
#endif
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
#if defined(__OOP_NOATTRBASES__) || defined(__OOP_NOMETHODBASES__)
            int fcount;
#endif
            #define OOPBase ad->OOPBase
            D(bug("[dosboot] %s: OOPBase @ 0x%p\n", __func__, OOPBase);)
            D(bug("[dosboot] %s: Bitmap @ 0x%p, hidd obj @ 0x%p\n", __func__, scr->RastPort.BitMap, HIDD_BM_OBJ(scr->RastPort.BitMap));)
#if defined(__OOP_NOATTRBASES__)
            if ((fcount = OOP_ObtainAttrBasesArray(&ad->bitMapAttrBase, attrbaseIDs)) == 0)
            {
                ad->ad_State |= STATEF_OOPATTRIBS;
#endif
#if defined(__OOP_NOMETHODBASES__)
                if ((fcount = OOP_ObtainMethodBasesArray(&ad->gfxMethodBase, methBaseIDs)) == 0)
                {
#endif
                    #undef HiddBitMapAttrBase
                    #define HiddBitMapAttrBase (ad->bitMapAttrBase)
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
                        ad->ad_State |= STATEF_POINTERHIDE;
                    }
#if defined(__OOP_NOMETHODBASES__)
                }
                else
                {
                    D(bug("[dosboot] %s: Failed to obtain %u Method Base(s)\n", __func__, fcount);)
                }
#endif
#if defined(__OOP_NOATTRBASES__)
            }
            else
            {
                D(bug("[dosboot] %s: Failed to obtain %u Attibute Base(s)\n", __func__, fcount);)
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
#if defined(__OOP_NOATTRBASES__)
        if (ad->ad_State & STATEF_OOPATTRIBS)
        {
            OOP_ReleaseAttrBasesArray(&ad->bitMapAttrBase, attrbaseIDs);
        }
#endif
        banm_Dispose(DOSBootBase);
    }
}
