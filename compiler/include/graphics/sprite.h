#ifndef	GRAPHICS_SPRITE_H
#define	GRAPHICS_SPRITE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sprite structures
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define SPRITE_ATTACHED 0x80

struct SimpleSprite
{
    UWORD *posctldata;
    UWORD height;
    UWORD   x,y;    
    UWORD   num;
};

struct ExtSprite
{
	struct SimpleSprite es_SimpleSprite;
	UWORD	es_wordwidth;
	UWORD	es_flags;

	/* New in AROS */
	struct BitMap * es_BitMap;  // Actual image data.
};



/* tags for use with AllocSpriteData() */
#define SPRITEA_Width		0x81000000
#define SPRITEA_XReplication	0x81000002
#define SPRITEA_YReplication	0x81000004
#define SPRITEA_OutputHeight	0x81000006
#define SPRITEA_Attached	0x81000008
#define SPRITEA_OldDataFormat	0x8100000a

/* tags valid for either GetExtSprite or ChangeExtSprite */
#define GSTAG_SCANDOUBLED	0x83000000

/* tags for use with GetExtSprite() */
#define GSTAG_SPRITE_NUM 0x82000020
#define GSTAG_ATTACHED	 0x82000022
#define GSTAG_SOFTSPRITE 0x82000024

#endif	/* GRAPHICS_SPRITE_H */
