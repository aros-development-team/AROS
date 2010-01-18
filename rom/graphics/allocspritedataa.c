/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AllocSpriteDataA()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/sprite.h>
#include <graphics/scale.h>
#include <utility/tagitem.h>
#include <exec/exec.h>
#include <string.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct ExtSprite *, AllocSpriteDataA,

/*  SYNOPSIS */
        AROS_LHA(struct BitMap *, bitmap, A2),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 170, Graphics)

/*  FUNCTION

    INPUTS
        bitmap - pointer to a bitmap. This bitmap provides the source data
                 for the sprite image
        tags   - pointer to a taglist

    TAGS
	SPRITEA_Width (ULONG)        - Width of the sprite. If bitmap is smaller it will
                                       be filled on the right side with transparent
                                       pixels. Defaults to 16.
	SPRITEA_XReplication (LONG)  -  0 - perform a 1 to 1 conversion
                                        1 - each pixel from the source is replicated twice
			                2 - each pixel is replicated 4 times.
		                       -1 - skip every 2nc pixel in the source bitmap
		                       -2 - only include every fourth pixel from the source.
	SPRITEA_YReplication (LONG)  - like SPRITEA_YReplication, but for vertical direction.
	SPRITEA_OutputHeight (ULONG) - Output height of the sprite. Must be at least as high
                                       as the bitmap. Defaults to bitmap height.
	SPRITEA_Attach               - (Not implemented)

    RESULT
        SpritePtr - pointer to a ExtSprite structure,
                    or NULL if there is a failure.
                    You should pass this pointer to FreeSpriteData()
                    when this sprite is not needed anymore.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ExtSprite *sprite = NULL;
    
    D(bug("AllocSpriteDataA(0x%08lX)\n", bitmap));
    if (NULL != bitmap) {
#define SCALE_NORMAL  16
        ULONG height = (ULONG)GetBitMapAttr(bitmap, BMA_HEIGHT);
        ULONG width = 16;
        const struct TagItem * tstate = tagList;
        struct TagItem * tag;
        struct BitScaleArgs bsa;
        LONG xrep = 0, yrep = 0;

        memset(&bsa, 0, sizeof(bsa));
    
        while (NULL != (tag = NextTagItem(&tstate))) {
            switch (tag->ti_Tag) {
                case SPRITEA_Width:
                	width = tag->ti_Data;
                break;
            
                case SPRITEA_XReplication:
                	xrep = tag->ti_Data;
                break;
            
                case SPRITEA_YReplication:
                	yrep = tag->ti_Data;
                break;
            
                case SPRITEA_OutputHeight:
                    if (tag->ti_Data > (ULONG)GetBitMapAttr(bitmap, BMA_HEIGHT)) {
                        return NULL;
                    }
                    height = tag->ti_Data;
                break;
            
                case SPRITEA_Attached:
                
                break;
            
            }
        }

        sprite = AllocVec(sizeof(*sprite), MEMF_PUBLIC | MEMF_CLEAR);
        if (NULL != sprite) {

            bsa.bsa_SrcWidth  = GetBitMapAttr(bitmap, BMA_WIDTH);
            bsa.bsa_SrcHeight = GetBitMapAttr(bitmap, BMA_HEIGHT);

	    D(bug("Source width %u Source height %u Sprite width %u Sprite height %u XReplication %u YReplication %u\n", bsa.bsa_SrcWidth, bsa.bsa_SrcHeight, width, height, xrep, yrep));
            if (xrep > 0) {
                bsa.bsa_XDestFactor = SCALE_NORMAL << xrep;
            } else {
                bsa.bsa_XDestFactor = SCALE_NORMAL >> (-xrep);
            }

            if (yrep > 0) {
                bsa.bsa_YDestFactor = SCALE_NORMAL << yrep;
            } else {
                bsa.bsa_YDestFactor = SCALE_NORMAL >> (-yrep);
            }

            bsa.bsa_XSrcFactor  = SCALE_NORMAL;
            bsa.bsa_YSrcFactor  = SCALE_NORMAL;
            bsa.bsa_SrcBitMap   = bitmap;
	    /* Graphics drivers expect mouse pointer bitmap in LUT8 format, so we give it */
            bsa.bsa_DestBitMap  = AllocBitMap(width, height, 4, BMF_CLEAR|BMF_SPECIALFMT|SHIFT_PIXFMT(PIXFMT_LUT8), NULL);
            BitMapScale(&bsa);
        
            sprite->es_SimpleSprite.height = height;
            sprite->es_SimpleSprite.x      = 0;
            sprite->es_SimpleSprite.y      = 0;
            sprite->es_SimpleSprite.num    = 0;
            sprite->es_wordwidth           = width >> 4;
            sprite->es_flags               = 0;
            sprite->es_BitMap              = bsa.bsa_DestBitMap;
            
	    D(bug("Allocated sprite data 0x%08lX: bitmap 0x%08lX, height %u\n", sprite, sprite->es_BitMap, sprite->es_SimpleSprite.height));
        }
    }
    return sprite;

    AROS_LIBFUNC_EXIT
} /* AllocSpriteDataA */
