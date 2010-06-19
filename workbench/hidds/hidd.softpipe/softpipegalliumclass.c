/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/gallium.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "softpipe/sp_texture.h"
#include "softpipe/sp_winsys.h"
#include "pipe/p_context.h"
#include "util/u_simple_screen.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_inlines.h"

#include "softpipe_intern.h"

#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <cybergraphx/cybergraphics.h>

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

#define CyberGfxBase    (&BASE(cl->UserData)->sd)->SoftpipeCyberGfxBase

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->hiddGalliumAB)

struct HiddSoftpipeBuffer
{
    struct pipe_buffer  base;
    APTR                buffer; /* Real buffer pointer */
    APTR                data;   /* Aligned buffer pointer (inside real buffer) */
    APTR                mapped;
};

struct HiddSoftpipeWinSys
{
    struct HIDDT_WinSys  base;
};

static struct pipe_buffer *
HiddSoftpipeBufferCreate(struct pipe_winsys *pws, 
                        unsigned alignment, 
                        unsigned usage,
                        unsigned size)
{
    struct HiddSoftpipeBuffer *buffer = AllocVec(sizeof(struct HiddSoftpipeBuffer), MEMF_PUBLIC|MEMF_CLEAR);

    pipe_reference_init(&buffer->base.reference, 1);
    buffer->base.alignment = alignment;
    buffer->base.usage = usage;
    buffer->base.size = size;
    if (buffer->buffer == NULL) {
        /* Alligment */
        buffer->buffer = AllocVec(size + alignment - 1, MEMF_PUBLIC);
        buffer->data = (void *)(((IPTR)buffer->buffer + (alignment - 1)) & ~(alignment - 1));
    }

    return &buffer->base;
}

static struct pipe_buffer *
HiddSoftpipeUserBufferCreate(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct HiddSoftpipeBuffer *buffer = AllocVec(sizeof(struct HiddSoftpipeBuffer), MEMF_PUBLIC|MEMF_CLEAR);
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = bytes;
   buffer->data = ptr;

   return &buffer->base;
}

static void *
HiddSoftpipeBufferMap(struct pipe_winsys *pws, struct pipe_buffer *buf,
              unsigned flags)
{
    struct HiddSoftpipeBuffer *hiddsoftpipebuffer = (struct HiddSoftpipeBuffer *)buf;
    hiddsoftpipebuffer->mapped = hiddsoftpipebuffer->data;
    return hiddsoftpipebuffer->mapped;
}

static void
HiddSoftpipeBufferUnmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
    struct HiddSoftpipeBuffer *hiddsoftpipebuffer = (struct HiddSoftpipeBuffer *)buf;
    hiddsoftpipebuffer->mapped = NULL;
}

static void
HiddSoftpipeBufferDestroy(struct pipe_buffer *buf)
{
    struct HiddSoftpipeBuffer *hiddsoftpipebuffer = (struct HiddSoftpipeBuffer *)buf;

    if (hiddsoftpipebuffer->buffer) {
        FreeVec(hiddsoftpipebuffer->buffer);  
        hiddsoftpipebuffer->data = NULL;
        hiddsoftpipebuffer->buffer = NULL;
    }

    FreeVec(hiddsoftpipebuffer);
}

static struct pipe_buffer *
HiddSoftpipeSurfaceBufferCreate(struct pipe_winsys *winsys,
                         unsigned width, unsigned height,
                         enum pipe_format format,
                         unsigned usage,
                         unsigned tex_usage,
                         unsigned *stride)
{
    const unsigned alignment = 64;
    unsigned nblocksy;

    nblocksy = util_format_get_nblocksy(format, height);
    *stride = align(util_format_get_stride(format, width), alignment);

    return winsys->buffer_create(winsys, alignment,
                                    usage,
                                    *stride * nblocksy);
}

static void 
HiddSoftpipeFlushFrontBuffer(struct pipe_winsys *ws,
                                struct pipe_surface *surf,
                                void *context_private)
{
    /* No Op */
}

static void 
HiddSoftpipeUpdateBuffer( struct pipe_winsys *ws, void *context_private )
{
    /* No Op */
}

static void
HiddSoftpipeDestroyWinSys( struct pipe_winsys *ws)
{
    FreeVec(ws);
}

static struct HiddSoftpipeWinSys *
HiddSoftpipeCreateSoftpipeWinSys( void )
{
    struct HiddSoftpipeWinSys * ws = NULL;

    ws = (struct HiddSoftpipeWinSys *)AllocVec
        (sizeof(struct HiddSoftpipeWinSys), MEMF_PUBLIC|MEMF_CLEAR);

    /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    */
    ws->base.base.buffer_create = HiddSoftpipeBufferCreate;
    ws->base.base.user_buffer_create = HiddSoftpipeUserBufferCreate;
    ws->base.base.buffer_map = HiddSoftpipeBufferMap;
    ws->base.base.buffer_unmap = HiddSoftpipeBufferUnmap;
    ws->base.base.buffer_destroy = HiddSoftpipeBufferDestroy;

    ws->base.base.surface_buffer_create = HiddSoftpipeSurfaceBufferCreate;

    ws->base.base.fence_reference = NULL; /* FIXME */
    ws->base.base.fence_signalled = NULL; /* FIXME */
    ws->base.base.fence_finish = NULL; /* FIXME */

    ws->base.base.flush_frontbuffer = HiddSoftpipeFlushFrontBuffer;
    ws->base.base.update_buffer = HiddSoftpipeUpdateBuffer;
    ws->base.base.get_name = NULL; /* FIXME */
    ws->base.base.destroy = HiddSoftpipeDestroyWinSys;

    return ws;
}

OOP_Object *METHOD(SoftpipeGallium, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID METHOD(SoftpipeGallium, Root, Get)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_GalliumInterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(SoftpipeGallium, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddSoftpipeWinSys * softpipews;
    struct pipe_screen *screen;

    softpipews = HiddSoftpipeCreateSoftpipeWinSys();
    if (softpipews == NULL)
        return NULL;

    screen = softpipe_create_screen((struct pipe_winsys *)softpipews);
    if (screen == NULL)
        goto fail;

    /* Preserve pointer to HIDD driver */
    softpipews->base.driver = o;

    return screen;

fail:
    if (softpipews && softpipews->base.base.destroy)
        softpipews->base.base.destroy((struct pipe_winsys *)softpipews);

    return NULL;
}

VOID METHOD(SoftpipeGallium, Hidd_Gallium, DisplaySurface)
{
    struct pipe_surface * surf = (struct pipe_surface *)msg->surface;
    struct softpipe_texture *spt = softpipe_texture(surf->texture);
    struct HiddSoftpipeBuffer *hiddsoftpipebuffer = (struct HiddSoftpipeBuffer *)(spt->buffer);
    struct RastPort * rp = CloneRastPort(msg->rastport);

    WritePixelArray(
        hiddsoftpipebuffer->data, 
        msg->left,
        msg->top,
        spt->stride[surf->level],
        rp, 
        msg->relx, 
        msg->rely, 
        msg->width, 
        msg->height, 
        AROS_PIXFMT);

    FreeRastPort(rp);
}

VOID METHOD(SoftpipeGallium, Hidd_Gallium, DestroyPipeScreen)
{
    struct pipe_screen * screen = (struct pipe_screen *)msg->screen;

    if (screen)
        screen->destroy(screen);
}
