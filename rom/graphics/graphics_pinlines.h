#ifndef _GFX_PINLINES_H
#define _GFX_PINLINES_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Private inlines for dos.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef GFX_BASE_NAME
#define GFX_BASE_NAME GfxBase
#endif

#define InitGfxHidd(hiddBase) \
    LP1( , BOOL, InitGfxHidd, \
	struct Library *, (hiddBase), a0, \
	GFX_BASE_NAME )

#endif _GFX_PINLINES_H
