#ifndef _GFX_PDEFS_H
#define _GFX_PDEFS_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
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

#endif /* _GFX_PDEFS_H */
