#ifndef _GFX_PRIVATE_H
#define _GFX_PRIVATE_H
/* 
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private function definitions for Gfx
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "graphics_pinlines.h"
#else
#   include "graphics_pdefs.h"
#endif

/*
    Prototypes
*/

AROS_LP1(BOOL, LateGfxInit,
    AROS_LPA(APTR, data, A0),
    struct GfxBase *, GfxBase, 181, Graphics)

AROS_LP1(struct BitMap *, AllocScreenBitMap,
    AROS_LPA(ULONG, modeid, D0),
    struct GfxBase *, GfxBase, 182, Graphics)

AROS_LP0(BOOL, MouseCoordsRelative,
    struct GfxBase *, GfxBase, 183, Graphics)


AROS_LP2(BOOL, SetFrontBitMap,
    AROS_LPA(struct BitMap *, bitmap, A0),
    AROS_LPA(BOOL, copyback, D0),
    struct GfxBase *, GfxBase, 184, Graphics)

AROS_LP2(VOID, SetPointerPos,
    AROS_LPA(UWORD, x, D0),
    AROS_LPA(UWORD, y, D1),
    struct GfxBase *, GfxBase, 185, Graphics)

AROS_LP5(VOID, SetPointerShape,
    AROS_LPA(UWORD *, shape, A0),
    AROS_LPA(UWORD, width, D0),
    AROS_LPA(UWORD, height, D1),
    AROS_LPA(UWORD, xoffset, D2),
    AROS_LPA(UWORD, yoffset, D3),
    struct GfxBase *, GfxBase, 186, Graphics)

#endif /* _GRAPHICS_PRIVATE_H */
