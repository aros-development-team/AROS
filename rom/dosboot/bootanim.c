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

//#define NOMEDIA_OLDSTYLE

#include "dosboot_intern.h"
#include "nomedia_image.h"
#include "nomedia_anim.h"

struct AnimData
{
    struct Library      *OOPBase;
#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase        bitMapAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID        gfxMethodBase;
#endif
    OOP_Object          *gfxhidd;
    UBYTE               **framedata;
    UWORD               x;
    UWORD               y;
    ULONG               tick, framecnt;
    UBYTE               frame;
};

#if defined(__OOP_NOMETHODBASES__)
static CONST_STRPTR const methBaseIDs[] =
{
    IID_Hidd_Gfx,
    NULL
};
#endif

static const UBYTE *unpack_byterun1(const UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for(;;)
    {
        c = (BYTE)(*source++);
        if (c >= 0)
        {
            while(c-- >= 0)
            {
                *dest++ = *source++;
                if (--unpackedsize <= 0) return source;
            }
        }
        else if (c != -128)
        {
            c = -c;
            r = *source++;

            while(c-- >= 0)
            {
                *dest++ = r;
                if (--unpackedsize <= 0) return source;
            }
        }
    }
}

#if !defined(NOMEDIA_OLDSTYLE)
static void WriteChunkRegion(struct DOSBootBase *DOSBootBase,
                                    struct RastPort *rp, UWORD x, UWORD y, UWORD width,
                                    UBYTE *data, UWORD regx, UWORD regy, UWORD regwidth, UWORD regheight)
{
    WriteChunkyPixels(rp, x + regx, y + regy,
                                    x + regx + regwidth - 1, y + regy + regheight - 1,
                                    data + regx + (regy * width), width);
}
#endif

APTR anim_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    D(bug("Screen: %dx%dx%d\nImage: %dx%dx%d\n",
        scr->Width, scr->Height, scr->RastPort.BitMap->Depth,
        NOMEDIA_WIDTH, NOMEDIA_HEIGHT, NOMEDIA_PLANES));
    if ((scr->Width >= NOMEDIA_WIDTH) && (scr->Height >= NOMEDIA_HEIGHT) &&
        (scr->RastPort.BitMap->Depth >= NOMEDIA_PLANES))
    {
        struct AnimData *ad = AllocMem(sizeof(struct AnimData) + (sizeof(struct picture *) *ANIMFRAME_COUNT), MEMF_ANY);
        ULONG size = NOMEDIA_WIDTH * NOMEDIA_HEIGHT;

        if (ad)
        {
            /* set up the date for the frames of the animation ..*/
            ad->framedata = (APTR)(IPTR)ad + sizeof(struct AnimData);
            ad->framedata[0] = AllocVec(size, MEMF_ANY);

            if (ad->framedata[0])
            {
                struct OOP_ABDescr attrbases[] = 
                {
                    { IID_Hidd_BitMap,  &ad->bitMapAttrBase     },
                    { NULL,             NULL                    }
                };
                ULONG i;
                ad->framedata[1] = (APTR)(IPTR)-1; // We just repeat the same frame...

                if ((ad->OOPBase = OpenLibrary("oop.library",0)) != NULL)
                {
                    #define OOPBase ad->OOPBase
                    bug("[dosboot] OOPBase @ 0x%p\n", OOPBase);
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
                        bug("[dosboot] Bitmap hidd @ 0x%p\n", HIDD_BM_OBJ(scr->RastPort.BitMap));
                        OOP_GetAttr(HIDD_BM_OBJ(scr->RastPort.BitMap), aHidd_BitMap_GfxHidd, (IPTR *)&ad->gfxhidd);
                        if (ad->gfxhidd){
                            bug("[dosboot] Gfx hidd @ 0x%p\n", ad->gfxhidd);
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

                unpack_byterun1(nomedia_data, ad->framedata[0], size);

                for (i = 0; i < NOMEDIA_COLORS; i++)
                    SetRGB32(&scr->ViewPort, i, (nomedia_pal[i] << 8) & 0xFF000000,
                                                (nomedia_pal[i] << 16) & 0xFF000000, (nomedia_pal[i] << 24) & 0xFF000000);

                SetAPen(&scr->RastPort, 0);
                RectFill(&scr->RastPort, 0, 0, scr->Width, scr->Height);

                ad->x = (scr->Width  - NOMEDIA_WIDTH ) >> 1;
                ad->y = (scr->Height - NOMEDIA_HEIGHT) >> 1;

#if defined(NOMEDIA_OLDSTYLE)
                WriteChunkyPixels(&scr->RastPort, ad->x, ad->y, ad->x + NOMEDIA_WIDTH - 1, ad->y + NOMEDIA_HEIGHT - 1,
                                  ad->framedata[0], NOMEDIA_WIDTH);
#else
                WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                            ad->framedata[0], BOOTLOGO_X, BOOTLOGO_Y, BOOTLOGO_WIDTH, BOOTLOGO_HEIGHT);
                WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                            ad->framedata[0], AROSLOGO_X, AROSLOGO_Y, AROSLOGO_WIDTH, AROSLOGO_HEIGHT);
                if ((ad->framedata[0] != ad->framedata[1]) && (ad->framedata[1] != (APTR)(IPTR)-1))
                {
                    WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                                ad->framedata[0], MEDIALOGO_X, MEDIALOGO_Y, MEDIALOGO_WIDTH, MEDIALOGO_HEIGHT);
                }
#endif
                ad->framecnt = ANIMFRAME_COUNT;
                ad->tick = 0;
                ad->frame = 0;
                DOSBootBase->animData = ad;
                DOSBootBase->delayTicks = 25;	/* We run at 2 frames per second */
                return ad;
            }
            FreeMem(ad, sizeof(struct AnimData));
        }
    }
    return NULL;
}

void anim_Animate(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;

    ad->frame = ad->tick++ % ad->framecnt;
    if (ad->frame)
        RectFill(&scr->RastPort, ad->x + FLASH_X, ad->y + FLASH_Y,
                 ad->x + FLASH_X + FLASH_WIDTH - 1, ad->y + FLASH_Y + FLASH_HEIGHT - 1);
    else
    {
        UBYTE *source;
        if (ad->framedata[ad->frame] == (APTR)(IPTR)-1)
            source = ad->framedata[0];
        else
            source = ad->framedata[ad->frame];
#if defined(NOMEDIA_OLDSTYLE)
        WriteChunkyPixels(&scr->RastPort, ad->x + FLASH_X, ad->y + FLASH_Y,
                          ad->x + FLASH_X + FLASH_WIDTH - 1, ad->y + FLASH_Y + FLASH_HEIGHT - 1,
                          source + FLASH_X, NOMEDIA_WIDTH);
#else
        WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                    source, FLASH_X, FLASH_Y, FLASH_WIDTH, FLASH_HEIGHT);
#endif
    }
}

void anim_Stop(struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;

    if (ad)
    {
        int frame;
        for (frame = 0; frame < ad->framecnt; frame ++)
        {
            if (ad->framedata[frame] != (APTR)(IPTR)-1)
                FreeVec(ad->framedata[frame]);
        }
#if (0)    
        struct pHidd_Gfx_SetCursorVisible p;
        p.mID = ad->gfxMethodBase + moHidd_Gfx_SetCursorVisible;
        p.visible = TRUE;
        (VOID)OOP_DoMethod(ad->gfxhidd, &p.mID);
#endif
        FreeMem(ad, sizeof(struct AnimData) + (sizeof(struct framedata *) * ad->framecnt));

        DOSBootBase->animData = NULL;

    }
}
