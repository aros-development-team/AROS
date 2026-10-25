#ifndef HIDD_GALLIUM_H
#define HIDD_GALLIUM_H

/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef _STDINT_H_
#   include <stdint.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

/* Gallium interface */
#define CLID_Hidd_Gallium   "hidd.gallium"
#define IID_Hidd_Gallium    "hidd.gallium"

#define HiddGalliumAttrBase __IHidd_Gallium

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddGalliumAttrBase;
#endif

/* Gallium Class methods */

enum
{
    moHidd_Gallium_CreatePipeScreen = 0,
    moHidd_Gallium_DestroyPipeScreen,
    moHidd_Gallium_DisplayResource,
    moHidd_Gallium_DisplayResourceRP,
    moHidd_Gallium_ResourceFromBitMap,

    NUM_GALLIUM_METHODS
};

enum
{
    aoHidd_Gallium_InterfaceVersion = 0,
    aoHidd_Gallium_CoreAPI,

    num_Hidd_Gallium_Attrs
};

#define aHidd_Gallium_InterfaceVersion  (HiddGalliumAttrBase + aoHidd_Gallium_InterfaceVersion)
#define aHidd_Gallium_CoreAPI  (HiddGalliumAttrBase + aoHidd_Gallium_CoreAPI)
#define aHidd_Gallium_WinSys  (HiddGalliumAttrBase + aoHidd_Gallium_WinSys)

#define IS_GALLIUM_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGalliumAttrBase) < num_Hidd_Gallium_Attrs)

struct pHidd_Gallium_CreatePipeScreen
{
    STACKED OOP_MethodID    mID;
};

struct pHidd_Gallium_DestroyPipeScreen
{
    STACKED OOP_MethodID    mID;
    STACKED APTR    screen;
};

struct pHidd_Gallium_DisplayResource
{
    STACKED OOP_MethodID            mID;
    STACKED APTR                    resource; // struct pipe_resource
    STACKED ULONG                   srcx;
    STACKED ULONG                   srcy;

    STACKED struct BitMap           *bitmap;
    STACKED ULONG                   dstx;
    STACKED ULONG                   dsty;
    STACKED ULONG                   width;
    STACKED ULONG                   height;
};

/* Whole-window present into a RastPort. A driver that can present the
 * resource itself (zero-copy overlay, scanout flip, or its own DMA blit)
 * overrides this and returns TRUE. The base class returns FALSE and the
 * caller (gallium.library BltPipeResourceRastPort) falls back to the
 * per-cliprect DisplayResource loop. */
struct pHidd_Gallium_DisplayResourceRP
{
    STACKED OOP_MethodID            mID;
    STACKED APTR                    resource; // struct pipe_resource
    STACKED LONG                    srcx;
    STACKED LONG                    srcy;

    STACKED struct RastPort         *rastport;
    STACKED LONG                    dstx;
    STACKED LONG                    dsty;
    STACKED LONG                    width;
    STACKED LONG                    height;
};

/* Alias one buffer of a scan-out bitmap (aHidd_BitMap_Scanout, created
 * through the screen's display) as a pipe_resource of the given pipe
 * screen, so that a GL context renders straight into display memory a
 * plane can scan out (or the compositor blits). The resource must use
 * the buffer's linear layout and pitch and be usable as a render target
 * (PIPE_BIND_RENDER_TARGET | SAMPLER_VIEW | SCANOUT | LINEAR). Returns a
 * referenced struct pipe_resource * (release with pipe_resource_reference)
 * or NULL. The base class returns NULL: gallium.library then refuses the
 * scan-out framebuffer and mesa3dgl keeps the DisplayResource blit path. */
struct pHidd_Gallium_ResourceFromBitMap
{
    STACKED OOP_MethodID            mID;
    STACKED OOP_Object              *bitmap;   /* scan-out bitmap object */
    STACKED ULONG                   buffer;    /* 0 .. aHidd_BitMap_Buffers-1 */
    STACKED APTR                    screen;    /* struct pipe_screen * of this pipe */
};

#endif
