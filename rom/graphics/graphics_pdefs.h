#ifndef _GFX_PDEFS_H
#define _GFX_PDEFS_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id

    Desc: Private function definitions for DOS
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

#define InitGfxHidd(hiddBase) \
    AROS_LC1(BOOL, InitGfxHidd, \
    AROS_LCA(struct Library *, hiddBase, A0), \
    struct GfxBase *, GfxBase, 181, Graphics)

#endif /* _GFX_PDEFS_H */
