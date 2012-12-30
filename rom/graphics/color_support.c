/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <graphics/view.h>

/*
** In case the representation of colors in the ColorTable of the color map
** are changed then this here should be the place where to change the
** algorithms.
*/

ULONG color_distance(struct ColorMap * cm,
                     ULONG r,
                     ULONG g,
                     ULONG b,
                     ULONG index)
{
    /* 
    ** I am assuming 24 bit colors that are represented in the color map as
    ** follows:
    ** cm->ColorTable is a pointer to an array of UWORDs where every
    **     UWORD contains the most significant 4 bits of each color.
    ** cm->LowColorBits is a pointer to an array of UWORDs where every
    **     UWORD contains the least significant 4 bits of each color.
    ** for example a color r=12, g=34, b=56 would be represented as:
    **
    **   cm->ColorTable[x]   = 0x0135
    **   cm->LowColorBits[x] = 0x0246
    **
    **   m68k graphics.library preserves high nibble of ColorTable value when changing it
    **   in SetRGB32cm() and SetRGB4cm() functions.
    **   Perhaps this has something to do with genlock support (alpha value?)
    **   Note that fields below ColorTable are present only if Type > COLORMAP_TYPE_V1_2
    */

    WORD dr,dg,db;

    UWORD c1 = cm->ColorTable[index];
    LONG r1 = (LONG)(c1 >> 4) & 0x00f0;
    LONG g1 = (LONG)(c1 >> 0) & 0x00f0;
    LONG b1 = (LONG)(c1 << 4) & 0x00f0;

    if (cm->Type > COLORMAP_TYPE_V1_2) {
        UWORD c2 = cm->LowColorBits[index];

	r1 |= (c2 >> 8) & 0x000f;
	g1 |= (c2 >> 4) & 0x000f;
	b1 |= (c2 >> 0) & 0x000f;
    }

    dr = (WORD)(r >> (32-8)) - (WORD)r1;
    dg = (WORD)(g >> (32-8)) - (WORD)g1;
    db = (WORD)(b >> (32-8)) - (WORD)b1;

    return (UWORD)(dr*dr)+(UWORD)(dg*dg)+(UWORD)(db*db);
}


/*
** Test whether the entry in the color map equals the given
** color
*/
BOOL color_equal(struct ColorMap * cm,
                 ULONG r,
                 ULONG g,
                 ULONG b,
                 ULONG index)
{
    if (cm->ColorTable[index] != (((r >> 20) & 0x0f00) |
                                              ((g >> 24) & 0x00f0) |
                                              ((b >> 28) & 0x000f)))
	return FALSE;

    if ((cm->Type > COLORMAP_TYPE_V1_2) &&
       cm->LowColorBits[index] != (((r >> 16) & 0x0f00) |
                                        	((g >> 20) & 0x00f0) |
                                        	((b >> 24) & 0x000f)))
        return FALSE;

    return TRUE;
}

VOID color_get(struct ColorMap *cm,
		ULONG *r,
		ULONG *g,
		ULONG *b,
		ULONG index)
{
    UWORD hibits = cm->ColorTable[index];

    ULONG red8   = (hibits & 0x0f00) >> 4;
    ULONG green8 = (hibits & 0x00f0);
    ULONG blue8  = (hibits & 0x000f) << 4;

    if (cm->Type > COLORMAP_TYPE_V1_2) {
        UWORD lobits = cm->LowColorBits[index];

	red8   |= (lobits & 0x0f00) >> 8;
        green8 |= (lobits & 0x00f0) >> 4;
        blue8  |= (lobits & 0x000f);
    }

    *r = red8   * 0x01010101;
    *g = green8 * 0x01010101;
    *b = blue8  * 0x01010101;
}
