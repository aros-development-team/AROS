#ifndef GRAPHICS_GFX_H
#define GRAPHICS_GFX_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphic structures
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

typedef UBYTE * PLANEPTR;
typedef struct tPoint
{
    WORD x;
    WORD y;
} Point;

#define BITSET 0x8000
#define BITCLR 0

#define AGNUS
#ifdef AGNUS
#define TOBB(x) ((LONG)(x))
#else
#define TOBB(x) ((LONG)(x)>>1)
#endif

struct BitMap
{
    UWORD    BytesPerRow;
    UWORD    Rows;
    UBYTE    Flags;
    UBYTE    Depth;
    UWORD    pad;
    PLANEPTR Planes[8];
};

#define RASSIZE(w,h)   ( (h) * ( ((w)+15) >>3 & 0xFFFE ))

struct Rectangle
{
    WORD MinX;
    WORD MinY;
    WORD MaxX;
    WORD MaxY;
};

struct Rect32
{
    LONG MinX;
    LONG MinY;
    LONG MaxX;
    LONG MaxY;
};

#define BMB_CLEAR            0
#define BMF_CLEAR       (1L<<0)
#define BMB_DISPLAYABLE      1
#define BMF_DISPLAYABLE (1L<<1)
#define BMB_INTERLEAVED      2
#define BMF_INTERLEAVED (1L<<2)
#define BMB_STANDARD         3
#define BMF_STANDARD    (1L<<3)
#define BMB_MINPLANES        4
#define BMF_MINPLANES   (1L<<4)

/* Cybergfx flag */
#define BMB_SPECIALFMT	     7
#define BMF_SPECIALFMT	(1L<<7)

#define BMB_PIXFMT_SHIFTUP 24

#define BMF_REQUESTVMEM (BMF_DISPLAYABLE|BMF_MINPLANES)

/* AmigaOS v4 flags*/
#define BMB_HIJACKED          7
#define BMF_HIJACKED     (1L<<7)
#define BMB_RTGTAGS           8
#define BMF_RTGTAGS	 (1L<<8)
#define BMB_RTGCHECK          9
#define BMF_RTGCHECK     (1L<<9)
#define BMB_FRIENDISTAG      10
#define BMF_FRIENDISTAG (1L<<10)
#define BMB_INVALID          11
#define BMF_INVALID     (1L<<11)

#define BMF_CHECKVALUE (BMF_RTGTAGS|BMF_RTGCHECK|BMF_FRIENDISTAG)
#define BMF_CHECKMASK  (BMF_HIJACKED|BMF_CHECKVALUE|BMF_INVALID)

#define BITMAPFLAGS_ARE_EXTENDED(f) ((f & BMF_CHECKMASK) == BMF_CHECKVALUE)

/* tags for AllocBitMap */
#define BMATags_Friend              (TAG_USER + 0)
#define BMATags_Depth               (TAG_USER + 1)
#define BMATags_RGBFormat           (TAG_USER + 2)
#define BMATags_Clear               (TAG_USER + 3)
#define BMATags_Displayable         (TAG_USER + 4)
#define BMATags_Private1            (TAG_USER + 5)
#define BMATags_NoMemory            (TAG_USER + 6)
#define BMATags_NoSprite            (TAG_USER + 7)
#define BMATags_Private2            (TAG_USER + 8)
#define BMATags_Private3            (TAG_USER + 9)
#define BMATags_ModeWidth           (TAG_USER + 10)
#define BMATags_ModeHeight          (TAG_USER + 11)
#define BMATags_RenderFunc          (TAG_USER + 12)
#define BMATags_SaveFunc            (TAG_USER + 13)
#define BMATags_UserData            (TAG_USER + 14)
#define BMATags_Alignment           (TAG_USER + 15)
#define BMATags_ConstantBytesPerRow (TAG_USER + 16)
#define BMATags_UserPrivate         (TAG_USER + 17)
#define BMATags_Private4            (TAG_USER + 18)
#define BMATags_Private5            (TAG_USER + 19)
#define BMATags_Private6            (TAG_USER + 20)
#define BMATags_Private7            (TAG_USER + 21)
#define BMATags_BitmapColors        (TAG_USER + 0x29)
#define BMATags_DisplayID           (TAG_USER + 0x32)
#define BMATags_BitmapInvisible     (TAG_USER + 0x37)
#define BMATags_BitmapColors32      (TAG_USER + 0x43)

/* IDs for GetBitMapAttr() */
#define BMA_HEIGHT 0
#define BMA_DEPTH  4
#define BMA_WIDTH  8
#define BMA_FLAGS  12

#endif /* GRAPHICS_GFX_H */
