#ifndef _GFX_PINLINES_H
#define _GFX_PINLINES_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private inlines for graphics.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef GFX_BASE_NAME
#define GFX_BASE_NAME GfxBase
#endif

#define LateGfxInit(data) \
    LP1( , BOOL, LateGfxInit, \
	APTR, data, a0, \
	GFX_BASE_NAME )

#define AllocScreenBitMap(modeid) \
    LP1( , struct BitMap *, AllocScreenBitMap, \
	ULONG, modeid, d0, \
	GFX_BASE_NAME )

#define MouseCoordsRelative() \
    LP0( , BOOL, MouseCoordsRelative, \
	GFX_BASE_NAME )

#define SetFrontBitMap(modeid) \
    LP2( , BOOL, SetFrontBitMap, \
	struct BitMap *, bitmap, a0, \
	BOOL, copyback, d0, \
	GFX_BASE_NAME )

#define SetPointerPos(x, y) \
    LP2( , VOID, SetPointerPos, \
	UWORD, x, d0, \
	UWORD, y, d1, \
	GFX_BASE_NAME )


#define SetPointerShape(shape, width, height, xoffset, yoffset) \
    LP5( , VOID, SetPointerShape, \
	UWORD *, shape, a0, \
	UWORD, width, d0, \
	UWORD, height, d1, \
	UWORD, xoffset, d2, \
	UWORD, yoffset, d3, \
	GFX_BASE_NAME )
	
#endif _GFX_PINLINES_H
