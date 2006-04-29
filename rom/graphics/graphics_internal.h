#ifndef GRAPHICS_INTERNAL_H
#define GRAPHICS_INTERNAL_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GfxHIDD specific Internal Information Management
    Lang: english
*/

#ifndef GRAPHICS_GFXBASE_H
#   include <graphics/gfxbase.h>
#endif
#ifndef HIDD_GRAPHICS_H
#   include <hidd/graphics.h>
#endif

#include "fakegfxhidd.h"
#include "objcache.h"


#if 0
VOID activatebm_callback(APTR data, OOP_Object *bmobj, BOOL activated);
BOOL init_activescreen_stuff(struct GfxBase *GfxBase);
VOID cleanup_activescreen_stuff(struct GfxBase *GfxBase);
#endif

#define PRIV_GFXBASE(base) ((struct GfxBase_intern *)base)

#include "macros.h"
#endif /* GRAPHICS_INTERNAL_H */
