#ifndef GELS_INTERNAL_H
#define GELS_INTERNAL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <graphics/gels.h>
#include <graphics/gfxbase.h>
#include <exec/types.h>

struct IntVSprite
{
	struct VSprite    * DrawPath;
	struct VSprite    * VSprite;       /* The VSprite this structure belongs to */
	struct BitMap     * ImageData;
	struct BitMap     * SaveBuffer;
	WORD              * OrigImageData; /* ImageData repesents OrigImageData which 
	                                      is taken from the VSprite ImageData.
	                                      This is used to detect changes in
	                                      the image data.
	                                    */
	WORD                Width;         /* The dimension of ImageData */
	WORD                Height;
	WORD                Depth;
};

struct IntVSprite * _CreateIntVSprite(struct VSprite * vs, 
                                      struct RastPort * rp,
                                      struct GfxBase * GfxBase);
VOID _DeleteIntVSprite(struct VSprite * vs,
                       struct GfxBase * GfxBase);
BOOL _ValidateIntVSprite(struct IntVSprite * ivs, 
                         struct RastPort * rp,
                         BOOL force_change,
                         struct GfxBase * GfxBase);
void _ClearBobAndFollowClearPath(struct VSprite * CurVSprite, 
                                 struct RastPort * rp,
                                 struct GfxBase * GfxBase);

#endif
