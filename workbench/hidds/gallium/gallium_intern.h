#ifndef _GALLIUM_INTERN_H
#define _GALLIUM_INTERN_H

/*
    Copyright 2010-2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdint.h>
#include <stdbool.h>

#include "pipe/p_shader_tokens.h"
#if defined(TGSI_SEMANTIC_VIEWPORT_MASK)
#include "frontend/sw_winsys.h"
#else
#include "state_tracker/sw_winsys.h"
#endif

#include LC_LIBDEFS_FILE

struct HiddGalliumData
{
    APTR  pad;
};

struct galliumstaticdata 
{
    OOP_Class       *galliumclass;
    OOP_AttrBase    galliumAttrBase;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct galliumstaticdata    sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif
