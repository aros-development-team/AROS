#ifndef _GFX_PINLINES_H
#define _GFX_PINLINES_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
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
	
#endif _GFX_PINLINES_H
