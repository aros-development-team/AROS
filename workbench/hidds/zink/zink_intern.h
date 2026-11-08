#ifndef _ZINK_INTERN_H
#define _ZINK_INTERN_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdint.h>
#include <stdbool.h>

#include <vulkan/vulkan_core.h>

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gallium_Zink  "hidd.gallium.zink"

struct pipe_screen;
struct pipe_context;

struct HiddGalliumZinkData
{
    OOP_Object                  *zink_obj;
    OOP_Object                  *zink_vkobj;     /* the hidd.vulkan object rendered through; owned */
    PFN_vkGetInstanceProcAddr   zink_gipa;       /* its vkGetInstanceProcAddr */
    struct pipe_screen          *zink_screen;    /* zink's own screen */
    struct pipe_screen          *client_screen;  /* the screen handed out by CreatePipeScreen: zink's,
                                                    or Mesa's trace driver wrapping it (GALLIUM_TRACE) */
    struct pipe_context         *zink_readctx;   /* context used for DisplayResource readback */
};

struct zinkstaticdata
{
    OOP_Class       *galliumclass;
    OOP_AttrBase    hiddGalliumAB;
    OOP_AttrBase    hiddBitMapAB;   /* scan-out bitmaps, for ResourceFromBitMap */
    OOP_AttrBase    hiddPixFmtAB;
    OOP_MethodID    bitmapMB;
    OOP_MethodID    vulkanMB;
    struct Library  *CyberGfxBase;
    struct Library  *UtilityBase;
};

LIBBASETYPE
{
    struct Library              LibNode;
    struct zinkstaticdata       sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#endif
