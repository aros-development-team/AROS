#ifndef _GFX_PDEFS_H
#define _GFX_PDEFS_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id

    Desc: Private function definitions for Gfx
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Defines
*/

#define LateGfxInit(data) \
    AROS_LC1(BOOL, LateGfxInit, \
    AROS_LCA(APTR, data, A0), \
    struct GfxBase *, GfxBase, 181, Graphics)

#define AllocScreenBitMap(modeid) \
    AROS_LC1(struct BitMap *, AllocScreenBitMap, \
    AROS_LCA(ULONG, modeid, D0), \
    struct GfxBase *, GfxBase, 182, Graphics)

#define MouseCoordsRelative() \
    AROS_LC0(BOOL, MouseCoordsRelative, \
    struct GfxBase *, GfxBase, 183, Graphics)


#define SetFrontBitMap(bitmap) \
    AROS_LC2(BOOL, SetFrontBitMap, \
    AROS_LCA(struct BitMap *, bitmap, A0), \
    AROS_LCA(BOOL, copyback, D0), \
    struct GfxBase *, GfxBase, 184, Graphics)
    

#define SetPointerPos(x, y) \
    AROS_LC2(VOID, SetPointerPos, \
    AROS_LCA(UWORD, x, D0), \
    AROS_LCA(UWORD, y, D1), \
    struct GfxBase *, GfxBase, 185, Graphics)
    
#define SetPointerShape(shape, width, height, xoffset, yoffset) \
    AROS_LC5(VOID, SetPointerShape, \
    AROS_LCA(UWORD *, shape, A0), \
    AROS_LCA(UWORD, width,  D0), \
    AROS_LCA(UWORD, height, D1), \
    AROS_LCA(UWORD, xoffset, D2), \
    AROS_LCA(UWORD, yoffset, D3), \
    struct GfxBase *, GfxBase, 186, Graphics)

    
#endif /* _GFX_PDEFS_H */
