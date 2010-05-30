/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa_softpipe.c 32712 2010-02-21 12:04:21Z deadwood $
*/

#include "arosmesa_winsys.h"
#include "softpipe/sp_texture.h"
#include "softpipe/sp_winsys.h"
#include "util/u_math.h"
#include "util/u_simple_screen.h"
#include "util/u_format.h"

#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

struct arosmesa_buffer
{
    struct pipe_buffer base;
    void *buffer; /* Real buffer pointer */
    void *data; /* Aligned buffer pointer (inside real buffer) */
    void *mapped;
};

/* FIXME: Don't use context. Use structure having access to rastport instead */
static void
arosmesa_softpipe_display_surface(AROSMesaContext amesa,
                              struct pipe_surface *surf)
{
   struct softpipe_texture *spt = softpipe_texture(surf->texture);
   struct arosmesa_buffer *amesa_buf = (struct arosmesa_buffer *)(spt->buffer);
     WritePixelArray(
     amesa_buf->data, 
        0,
        0,
        spt->stride[surf->level],
        amesa->visible_rp, 
        amesa->left, 
        amesa->top, 
        amesa->width, 
        amesa->height, 
        AROS_PIXFMT);
}

static struct pipe_buffer *
arosmesa_buffer_create(struct pipe_winsys *pws, 
                        unsigned alignment, 
                        unsigned usage,
                        unsigned size)
{
    struct arosmesa_buffer *buffer = AllocVec(sizeof(struct arosmesa_buffer), MEMF_PUBLIC|MEMF_CLEAR);

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
arosmesa_user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
   struct arosmesa_buffer *buffer = AllocVec(sizeof(struct arosmesa_buffer), MEMF_PUBLIC|MEMF_CLEAR);
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = bytes;
   buffer->data = ptr;

   return &buffer->base;
}

static void *
arosmesa_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
              unsigned flags)
{
    struct arosmesa_buffer *amesa_buf = (struct arosmesa_buffer *)buf;
    amesa_buf->mapped = amesa_buf->data;
    return amesa_buf->mapped;
}

static void
arosmesa_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
    struct arosmesa_buffer *amesa_buf = (struct arosmesa_buffer *)buf;
    amesa_buf->mapped = NULL;
}

static void
arosmesa_buffer_destroy(struct pipe_buffer *buf)
{
    struct arosmesa_buffer *amesa_buf = (struct arosmesa_buffer *)buf;

    if (amesa_buf->buffer) {
        FreeVec(amesa_buf->buffer);  
        amesa_buf->data = NULL;
        amesa_buf->buffer = NULL;
    }

    FreeVec(amesa_buf);
}

static struct pipe_buffer *
arosmesa_surface_buffer_create(struct pipe_winsys *winsys,
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
arosmesa_softpipe_flush_frontbuffer(struct pipe_winsys *ws,
                                struct pipe_surface *surf,
                                void *context_private)
{
    /* No Op */
}

static void 
arosmesa_softpipe_update_buffer( struct pipe_winsys *ws, void *context_private )
{
    /* No Op */
}

static void
arosmesa_softpipe_destroy( struct pipe_winsys *ws)
{
    FreeVec(ws);
}

static struct pipe_winsys *
arosmesa_create_softpipe_winsys( void )
{
    struct pipe_winsys *ws = NULL;

    ws = (struct pipe_winsys *)AllocVec(sizeof(struct pipe_winsys), MEMF_PUBLIC|MEMF_CLEAR);

    /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    */
    ws->buffer_create = arosmesa_buffer_create;
    ws->user_buffer_create = arosmesa_user_buffer_create;
    ws->buffer_map = arosmesa_buffer_map;
    ws->buffer_unmap = arosmesa_buffer_unmap;
    ws->buffer_destroy = arosmesa_buffer_destroy;

    ws->surface_buffer_create = arosmesa_surface_buffer_create;

    ws->fence_reference = NULL; /* FIXME */
    ws->fence_signalled = NULL; /* FIXME */
    ws->fence_finish = NULL; /* FIXME */

    ws->flush_frontbuffer = arosmesa_softpipe_flush_frontbuffer;
    ws->update_buffer = arosmesa_softpipe_update_buffer;
    ws->get_name = NULL; /* FIXME */
    ws->destroy = arosmesa_softpipe_destroy;

    return ws;
}


static struct pipe_screen *
arosmesa_create_softpipe_screen( void )
{
    struct pipe_winsys *winsys;
    struct pipe_screen *screen;

    winsys = arosmesa_create_softpipe_winsys();
    if (winsys == NULL)
        return NULL;

    screen = softpipe_create_screen(winsys);
    if (screen == NULL)
        goto fail;

    return screen;

fail:
    if (winsys && winsys->destroy)
        winsys->destroy( winsys );

    return NULL;
}


static struct pipe_context *
arosmesa_create_softpipe_context( struct pipe_screen *screen )
{
    if (!screen)
        return NULL;

    return screen->context_create(screen, NULL);
}

static void
arosmesa_softpipe_query_depth_stencil(int color, int * depth, int * stencil)
{
    (*depth)    = 16;
    (*stencil)  = 8;
}

static struct pipe_surface *
arosmesa_softpipe_get_screen_surface(struct pipe_screen * screen, int width, int height, int bpp)
{
    /* No op */
    return NULL;
}

static void
arosmesa_softpipe_cleanup( struct pipe_screen * screen )
{
    if (screen)
    {
        screen->destroy(screen);
        /* screen->destroy calls winsys->destroy */
    }
}

struct arosmesa_driver arosmesa_softpipe_driver = 
{
    .create_pipe_screen = arosmesa_create_softpipe_screen,
    .create_pipe_context = arosmesa_create_softpipe_context,
    .display_surface = arosmesa_softpipe_display_surface,
    .query_depth_stencil = arosmesa_softpipe_query_depth_stencil,
    .get_screen_surface = arosmesa_softpipe_get_screen_surface,
    .cleanup = arosmesa_softpipe_cleanup,
};



