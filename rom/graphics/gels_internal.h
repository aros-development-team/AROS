#ifndef GELS_INTERNAL_H
#define GELS_INTERNAL_H

#include <graphics/gels.h>
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

struct IntVSprite * _CreateIntVSprite(struct VSprite * vs, struct RastPort * rp);
VOID _DeleteIntVSprite(struct VSprite * vs);
BOOL _ValidateIntVSprite(struct IntVSprite * ivs, 
                         struct RastPort * rp,
                         BOOL force_change);


#endif
