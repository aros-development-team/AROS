#ifndef _GFX_PRIVATE_H
#define _GFX_PRIVATE_H
/* 
    Copyright (C) 1998 AROS - The Amiga Research OS
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

AROS_LP1(BOOL, InitGfxHidd,
    AROS_LPA(struct Library *, hiddBase, A0),
    struct GfxBase *, GfxBase, 181, Dos)

#endif /* _GRAPHICS_PRIVATE_H */
