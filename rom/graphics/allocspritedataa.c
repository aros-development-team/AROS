/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AllocSpriteDataA()
    Lang: english
*/
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <aros/debug.h>
#include <graphics/gfx.h>
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
    
    if (NULL != bitmap) {
#define SCALE_NORMAL  16
        ULONG height = (ULONG)GetBitMapAttr(bitmap, BMA_HEIGHT);
        ULONG width = 16;
        struct TagItem * tstate = tagList;
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
            bsa.bsa_DestBitMap  = AllocBitMap(width,
                                              height,
                                              4,
                                              BMF_CLEAR|BMF_DISPLAYABLE,
                                              NULL);
            BitMapScale(&bsa);
        
            sprite->es_SimpleSprite.height = height;
            sprite->es_SimpleSprite.x      = 0;
            sprite->es_SimpleSprite.y      = 0;
            sprite->es_SimpleSprite.num    = 0;
            sprite->es_wordwidth           = width >> 4;
            sprite->es_flags               = 0;
            sprite->es_BitMap              = bsa.bsa_DestBitMap;
            
        }
    }
    return sprite;

    AROS_LIBFUNC_EXIT
} /* AllocSpriteDataA */
