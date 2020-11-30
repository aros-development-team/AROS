
#define __OOP_NOLIBBASE__
#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__

#include <aros/debug.h>
#include <proto/graphics.h>

//#define NOMEDIA_OLDSTYLE

#include "nomedia_image.h"
#include "nomedia_anim.h"

#include "bootanim.h"

#define STATEF_WINK             (1 << 0)
#define STATEF_AROSLOGO         (1 << 1)

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

struct AnimData *banm_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = NULL;

    D(
        bug("%s(0x%p, 0x%p)\n", __func__, scr, DOSBootBase);
        bug("%s:%dx%dx%d Image\n", __func__, NOMEDIA_WIDTH, NOMEDIA_HEIGHT, NOMEDIA_PLANES);
    )

    if ((scr->Width >= NOMEDIA_WIDTH) && (scr->Height >= NOMEDIA_HEIGHT) &&
        (scr->RastPort.BitMap->Depth >= NOMEDIA_PLANES))
    {
        ULONG size = NOMEDIA_WIDTH * NOMEDIA_HEIGHT;

        ad = AllocMem(sizeof(struct AnimData) + (sizeof(struct picture *) *ANIMFRAME_COUNT), MEMF_ANY);
        if (ad)
        {
            ad->x = (scr->Width  - NOMEDIA_WIDTH ) >> 1;
            ad->y = (scr->Height - NOMEDIA_HEIGHT) >> 1;
            ad->framecnt = ANIMFRAME_COUNT;
            ad->tick = 0;
            ad->frame = 0;
            ad->ad_State = 0;

            /* set up the date for the frames of the animation ..*/
            ad->framedata = (APTR)(IPTR)ad + sizeof(struct AnimData);
            ad->framedata[0] = AllocVec(size, MEMF_ANY);

            if (ad->framedata[0])
            {
                ULONG i;

                ad->framedata[1] = (APTR)(IPTR)-1; // We just repeat the same frame...

                unpack_byterun1(nomedia_data, ad->framedata[0], size);

                for (i = 0; i < NOMEDIA_COLORS; i++)
                    SetRGB32(&scr->ViewPort, i, (nomedia_pal[i] << 8) & 0xFF000000,
                                                (nomedia_pal[i] << 16) & 0xFF000000, (nomedia_pal[i] << 24) & 0xFF000000);
            }
        }
    }
    return ad;
}

void banm_Dispose(struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;

    D(bug("%s()\n", __func__));

    if (ad)
    {
        int frame;
        DOSBootBase->animData = NULL;
        for (frame = 0; frame < ad->framecnt; frame ++)
        {
            if (ad->framedata[frame] != (APTR)(IPTR)-1)
                FreeVec(ad->framedata[frame]);
        }
        FreeMem(ad, sizeof(struct AnimData) + (sizeof(struct framedata *) * ad->framecnt));
    }
}

void banm_ShowFrame(struct Screen *scr, BOOL init, struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;
    if (init)
    {
        D(bug("%s(init)\n", __func__);)
 #if defined(NOMEDIA_OLDSTYLE)
        WriteChunkyPixels(&scr->RastPort, ad->x, ad->y, ad->x + NOMEDIA_WIDTH - 1, ad->y + NOMEDIA_HEIGHT - 1,
                          ad->framedata[0], NOMEDIA_WIDTH);
#else
        WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                    ad->framedata[0], BOOTLOGO_X, BOOTLOGO_Y, BOOTLOGO_WIDTH, BOOTLOGO_HEIGHT);
#endif
    }
    else
    {
        D(
            bug("%s(0x%p, 0x%p)\n", __func__, scr, DOSBootBase);
            bug("%s: frame %u, tick %u\n", __func__, ad->frame, ad->tick);
        )
        /* Animate Wink */
        if (ad->tick < 2)
        {
            RectFill(&scr->RastPort, ad->x + BOOTLOGO_X + (BOOTLOGO_WIDTH >> 1), ad->y + BOOTLOGO_Y,
                     ad->x + BOOTLOGO_X + BOOTLOGO_WIDTH - 1, ad->y + BOOTLOGO_Y + BOOTLOGO_HEIGHT - 1);
        }
        else
        {
            if (!(ad->ad_State & STATEF_WINK))
            {
                ad->ad_State |= STATEF_WINK;    
                WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                    ad->framedata[0], BOOTLOGO_X + (BOOTLOGO_WIDTH >> 1), BOOTLOGO_Y, (BOOTLOGO_WIDTH >> 1), BOOTLOGO_HEIGHT);
            }
            /* Show Logo */
            if ((ad->tick > 3) && !(ad->ad_State & STATEF_AROSLOGO))
            {
                ad->ad_State |= STATEF_AROSLOGO;
                WriteChunkRegion(DOSBootBase, &scr->RastPort, ad->x, ad->y, NOMEDIA_WIDTH,
                                    ad->framedata[0], AROSLOGO_X, AROSLOGO_Y, AROSLOGO_WIDTH, AROSLOGO_HEIGHT);
            }
        }

        /* Animate media region */
        if (ad->frame)
        {
            RectFill(&scr->RastPort, ad->x + FLASH_X, ad->y + FLASH_Y,
                     ad->x + FLASH_X + FLASH_WIDTH - 1, ad->y + FLASH_Y + FLASH_HEIGHT - 1);
        }
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
}
