/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This animation code draws a picture centered on the screen
 * and flashes a specified region once per second.
 */

#include <aros/debug.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include "dosboot_intern.h"
#include "nomedia_image.h"
#include "nomedia_anim.h"

struct AnimData
{
    UBYTE *picture;
    UBYTE frame;
    UWORD x;
    UWORD y;
};

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

APTR anim_Init(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    D(bug("Screen: %dx%dx%d\nImage: %dx%dx%d\n",
    	scr->Width, scr->Height, scr->RastPort.BitMap->Depth,
    	NOMEDIA_WIDTH, NOMEDIA_HEIGHT, NOMEDIA_PLANES));
    if ((scr->Width >= NOMEDIA_WIDTH) && (scr->Height >= NOMEDIA_HEIGHT) &&
        (scr->RastPort.BitMap->Depth >= NOMEDIA_PLANES))
    {
        struct AnimData *ad = AllocMem(sizeof(struct AnimData), MEMF_ANY);
        ULONG size = NOMEDIA_WIDTH * NOMEDIA_HEIGHT;
        
        if (ad)
        {
            ad->picture = AllocVec(size, MEMF_ANY);
            
            if (ad->picture)
            {
		ULONG i;

		unpack_byterun1(nomedia_data, ad->picture, size);

		for (i = 0; i < NOMEDIA_COLORS; i++)
		    SetRGB32(&scr->ViewPort, i, (nomedia_pal[i] << 8) & 0xFF000000,
						(nomedia_pal[i] << 16) & 0xFF000000, (nomedia_pal[i] << 24) & 0xFF000000);

		SetAPen(&scr->RastPort, 0);
		RectFill(&scr->RastPort, 0, 0, scr->Width, scr->Height);

          	ad->x = (scr->Width  - NOMEDIA_WIDTH ) >> 1;
		ad->y = (scr->Height - NOMEDIA_HEIGHT) >> 1;
		WriteChunkyPixels(&scr->RastPort, ad->x, ad->y, ad->x + NOMEDIA_WIDTH - 1, ad->y + NOMEDIA_HEIGHT - 1,
				  ad->picture, NOMEDIA_WIDTH);

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
    
    ad->frame = !ad->frame;
    if (ad->frame)
    	RectFill(&scr->RastPort, ad->x + FLASH_X, ad->y + FLASH_Y,
    		 ad->x + FLASH_X + FLASH_WIDTH - 1, ad->y + FLASH_Y + FLASH_HEIGHT - 1);
    else
    {
    	WriteChunkyPixels(&scr->RastPort, ad->x + FLASH_X, ad->y + FLASH_Y,
    			  ad->x + FLASH_X + FLASH_WIDTH - 1, ad->y + FLASH_Y + FLASH_HEIGHT - 1,
    			  ad->picture + FLASH_X, NOMEDIA_WIDTH);
    }
}

void anim_Stop(struct DOSBootBase *DOSBootBase)
{
    struct AnimData *ad = DOSBootBase->animData;
    
    if (ad)
    {
    	FreeVec(ad->picture);
    	FreeMem(ad, sizeof(struct AnimData));
    
    	DOSBootBase->animData = NULL;
    }
}
