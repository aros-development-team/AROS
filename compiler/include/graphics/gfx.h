#ifndef GRAPHICS_GFX_H
#define GRAPHICS_GFX_H

/*
    (C) 1997 AROS - The Amiga Research OS
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
    UWORD    Pad;
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

#define BMB_AROS_HIDD        7
#define BMF_AROS_HIDD	(1L << 7)

#define BMA_HEIGHT 0
#define BMA_DEPTH  4
#define BMA_WIDTH  8
#define BMA_FLAGS  12

#endif /* GRAPHICS_GFX_H */
