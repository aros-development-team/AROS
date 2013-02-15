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
#include <hidd/graphics.h>
#include <utility/tagitem.h>
#include <exec/exec.h>
#include <proto/oop.h>
#include <string.h>
#include "gfxfuncsupport.h"
#include "graphics_intern.h"

#if DEBUG

#define PRINT_PIXFMT(bitmap)						\
if (bitmap->Flags & BMF_SPECIALFMT) {					\
    OOP_Object *pf;							\
    IPTR stdpf;								\
									\
    OOP_GetAttr(HIDD_BM_OBJ(bitmap), aHidd_BitMap_PixFmt, (IPTR *)&pf);	\
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);			\
									\
    bug("[AllocSpriteData] Bitmap pixelformat: %lu\n", stdpf);		\
}

#else
#define PRINT_PIXFMT(bitmap)
#endif

#if DEBUG > 1

#define PRINT_BITMAP(bitmap, xmax, ymax)			\
bug("[AllocSpriteData] Bitmap contents:\n");			\
{								\
    OOP_Object *bm = OBTAIN_HIDD_BM(bitmap);			\
    ULONG x, y;							\
								\
    for (y = 0; y < ymax; y++) {				\
        for (x = 0; x < xmax; x++) {				\
            HIDDT_Pixel pix = HIDD_BM_GetPixel(bm, x, y);	\
								\
	    bug("0x%08lX ", pix);				\
	}							\
	bug("\n");						\
    }								\
    RELEASE_HIDD_BM(bm, bitmap);				\
}

#else
#define PRINT_BITMAP(bitmap, xmax, ymax)
#endif

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
	BOOL have_OutputHeight = FALSE;
	BOOL have_OldDataFormat = FALSE;
        ULONG height = 0;
        ULONG width = 16;
        struct TagItem * tag, * tstate = tagList;
	struct BitMap *friend_bm = NULL;
	ULONG pixfmt = BMF_SPECIALFMT|SHIFT_PIXFMT(PIXFMT_LUT8);
	struct BitMap old_bitmap;
	UWORD *planes = NULL;
	ULONG planes_size = 0;
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
                    height = tag->ti_Data;
		    have_OutputHeight = TRUE;
                break;

		/* Currently we don't support this */
                case SPRITEA_Attached:
		    return 0;
                
		case SPRITEA_OldDataFormat:
		    have_OldDataFormat = TRUE;
            }
        }
	
	if (have_OldDataFormat) {
	    /* A zero-width/height sprite is evidently legal on AOS */
	    UWORD height2 = height == 0 ? 1 : height;
	    UWORD *p, *q, *s = (UWORD *)bitmap + 2;
	    UWORD k, mask;

	    InitBitMap(&old_bitmap, 2, 16, height2);
	    planes_size = height2 * sizeof(UWORD) * 2;
	    planes = AllocMem(planes_size, MEMF_CLEAR | MEMF_CHIP);
	    if (!planes)
		return NULL;
	    p = planes;
	    q = &planes[height2];
	    mask = ~((1 << (16 - width)) - 1);
	    old_bitmap.Planes[0] = (PLANEPTR)p;
	    old_bitmap.Planes[1] = (PLANEPTR)q;
	    for (k = 0; k < height; ++k) {
		*p++ = AROS_WORD2BE(*s++ & mask);
		*q++ = AROS_WORD2BE(*s++ & mask);
	    }
	    height = height2;
	    width = 16;
	    bsa.bsa_SrcBitMap = &old_bitmap;
	    bsa.bsa_SrcWidth = width;
	    bsa.bsa_SrcHeight = height;
	} else {
	    bsa.bsa_SrcBitMap   = bitmap;
	    bsa.bsa_SrcWidth  = GetBitMapAttr(bitmap, BMA_WIDTH);
	    bsa.bsa_SrcHeight = GetBitMapAttr(bitmap, BMA_HEIGHT);
	    if (have_OutputHeight) {
	        if (height > bsa.bsa_SrcHeight)
	            return NULL;
	    } else
	        height = bsa.bsa_SrcHeight;

	    /* This is a part of experimental truecolor pointer support.

	       Check if the source bitmap is a HIDD bitmap. If so, we do
	       not specify pixelformat and take if from original bitmap
	       instead. 
	       In fact the whole trick is a temporary hack. Old display
	       drivers will fail to set or display pointer sprite if
	       the supplied bitmap is not in LUT8 format. This is wrong
	       by itself and needs to be fixed. */
	    if (IS_HIDD_BM(bitmap)) {
	        friend_bm = bitmap;
		pixfmt = 0;
	    }
	}

	D(bug("[AllocSpriteData] Source bitmap depth: %u\n", GetBitMapAttr(bsa.bsa_SrcBitMap, BMA_DEPTH)));
        PRINT_PIXFMT(bsa.bsa_SrcBitMap);
	PRINT_BITMAP(bsa.bsa_SrcBitMap, 8, 8);

        sprite = AllocVec(sizeof(*sprite), MEMF_PUBLIC | MEMF_CLEAR);
        if (NULL != sprite) {

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
	    /* Graphics drivers expect mouse pointer bitmap in LUT8 format, so we give it */
            bsa.bsa_DestBitMap  = AllocBitMap(width, height, 8, BMF_CLEAR|pixfmt, friend_bm);
	    if (bsa.bsa_DestBitMap) {
		BitMapScale(&bsa);
        
		sprite->es_SimpleSprite.height = height;
		sprite->es_SimpleSprite.x      = 0;
		sprite->es_SimpleSprite.y      = 0;
		sprite->es_SimpleSprite.num    = 0;
		sprite->es_wordwidth           = width >> 4;
		sprite->es_flags               = 0;
		sprite->es_BitMap              = bsa.bsa_DestBitMap;
            
		D(bug("[AllocSpriteData] Allocated sprite data 0x%08lX: bitmap 0x%08lX, height %u\n", sprite, sprite->es_BitMap, sprite->es_SimpleSprite.height));
		D(bug("[AllocSpriteData] Bitmap depth: %u\n", GetBitMapAttr(bsa.bsa_DestBitMap, BMA_DEPTH)));
		PRINT_PIXFMT(bsa.bsa_DestBitMap);
		PRINT_BITMAP(bsa.bsa_DestBitMap, 8, 8);
	    } else {
	        FreeVec(sprite);
		sprite = NULL;
	    }
        }
	if (have_OldDataFormat && planes_size)
	    FreeMem(planes, planes_size);
    }
    return sprite;

    AROS_LIBFUNC_EXIT
} /* AllocSpriteDataA */
