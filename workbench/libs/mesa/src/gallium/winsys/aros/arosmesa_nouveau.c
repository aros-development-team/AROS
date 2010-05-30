/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa_nouveau.c 32721 2010-02-23 19:39:39Z deadwood $
*/

#include "arosmesa_winsys.h"
#include "nouveau/nouveau_winsys.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "nouveau/nouveau_screen.h"

#include "nouveau/nouveau_drmif.h"

#define DEBUG 0
#include <aros/debug.h>

#include <proto/graphics.h>
#include <proto/layers.h>

struct nouveau_winsys {
    struct pipe_winsys base;

    struct pipe_screen *pscreen;

    unsigned nr_pctx;
    struct pipe_context **pctx;

    struct pipe_surface *front;
};

static INLINE struct nouveau_winsys *
nouveau_winsys(struct pipe_winsys *ws)
{
    return (struct nouveau_winsys *)ws;
}

static INLINE struct nouveau_winsys *
nouveau_winsys_screen(struct pipe_screen *pscreen)
{
    return nouveau_winsys(pscreen->winsys);
}

static void
arosmesa_nouveau_display_surface(AROSMesaContext amesa,
                              struct pipe_surface *surf)
{
    struct pipe_context *pipe = amesa->st->pipe;
    
    struct Layer *L = amesa->visible_rp->Layer;
    struct ClipRect *CR;
    struct Rectangle renderableLayerRect;

    if (amesa->screen_surface == NULL)
    {
        D(bug("Screen surface not provided\n"));
        return;
    }
    
    if (L == NULL)
    {
        D(bug("Layer not provided\n"));
        /* FIXME: Implement rendering path - render at 0,0 full size of buffer? */
        return;
    }

    if (!IsLayerVisible(L))
        return;

    LockLayerRom(L);
    
    renderableLayerRect.MinX = L->bounds.MinX + amesa->left;
    renderableLayerRect.MaxX = L->bounds.MaxX - amesa->right;
    renderableLayerRect.MinY = L->bounds.MinY + amesa->top;
    renderableLayerRect.MaxY = L->bounds.MaxY - amesa->bottom;
    
    /*  Do not clip renderableLayerRect to screen rast port. CRs are already clipped and unclipped 
        layer coords are needed: see surface_copy */
    
    CR = L->ClipRect;
    
    for (;NULL != CR; CR = CR->Next)
    {
/*        D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
            CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
            CR->lobs));*/

        /* I assume this means the cliprect is visible */
        if (NULL == CR->lobs)
        {
            struct Rectangle result;
            
            if (AndRectRect(&renderableLayerRect, &CR->bounds, &result))
            {
                /* This clip rect intersects renderable layer rect */
                
                /* FIXME: clip last 4 parameters to actuall surface deminsions */
                pipe->surface_copy(pipe, amesa->screen_surface, 
                            result.MinX, 
                            result.MinY, 
                            surf, 
                            result.MinX - L->bounds.MinX - amesa->left, 
                            result.MinY - L->bounds.MinY - amesa->top, 
                            result.MaxX - result.MinX + 1, 
                            result.MaxY - result.MinY + 1);
            }
        }
    }

    /* Flush all copy operations done */
    pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

    UnlockLayerRom(L);
}

static int
arosmesa_open_nouveau_device(struct nouveau_device **dev)
{
    return nouveau_device_open(dev, "");
}

static void 
arosmesa_nouveau_flush_frontbuffer( struct pipe_screen *screen,
                            struct pipe_surface *surf,
                            void *context_private )
{
    /* No Op */
}

static void
arosmesa_nouveau_destroy(struct pipe_winsys *ws)
{
    struct nouveau_winsys *nvws = nouveau_winsys(ws);

    nouveau_device_close(&(nouveau_screen(nvws->pscreen)->device));
    FREE(nvws);
}


static struct pipe_screen *
arosmesa_create_nouveau_screen( void )
{
    struct nouveau_winsys *nvws;
    struct pipe_winsys *ws;
    struct nouveau_device *dev = NULL;
    struct pipe_screen *(*init)(struct pipe_winsys *,
                    struct nouveau_device *);
    int ret;

    ret = arosmesa_open_nouveau_device(&dev);
    if (ret)
        return NULL;

    switch (dev->chipset & 0xf0) {
    case 0x30:
        init = nv30_screen_create;
        break;
    case 0x40:
    case 0x60:
        init = nv40_screen_create;
        break;
    case 0x80:
    case 0x90:
    case 0xa0:
        init = nv50_screen_create;
        break;
    default:
        debug_printf("%s: unknown chipset nv%02x\n", __func__,
                 dev->chipset);
        return NULL;
    }


    nvws = CALLOC_STRUCT(nouveau_winsys);
    if (!nvws) {
        nouveau_device_close(&dev);
        return NULL;
    }
    ws = &nvws->base;
    ws->destroy = arosmesa_nouveau_destroy;

    nvws->pscreen = init(ws, dev);
    if (!nvws->pscreen) {
        ws->destroy(ws);
        return NULL;
    }
    
    nvws->pscreen->flush_frontbuffer = arosmesa_nouveau_flush_frontbuffer;
    
    return nvws->pscreen;
}

static struct pipe_context *
arosmesa_create_nouveau_context( struct pipe_screen *pscreen )
{
    if (!pscreen)
        return NULL;

    return pscreen->context_create(pscreen, NULL);
}

static struct pipe_buffer *
nouveau_drm_pb_from_handle(struct pipe_screen *pscreen, unsigned int handle)
{
    struct nouveau_device *dev = nouveau_screen(pscreen)->device;
    struct pipe_buffer *pb;
    int ret;

    pb = CALLOC(1, sizeof(struct pipe_buffer) + sizeof(struct nouveau_bo*));
    if (!pb)
        return NULL;

    ret = nouveau_bo_handle_ref(dev, handle, (struct nouveau_bo**)(pb+1));
    if (ret) {
        debug_printf("%s: ref name 0x%08x failed with %d\n",
                 __func__, handle, ret);
        FREE(pb);
        return NULL;
    }

    pipe_reference_init(&pb->reference, 1);
    pb->screen = pscreen;
    pb->alignment = 0;
    pb->usage = PIPE_BUFFER_USAGE_GPU_READ_WRITE |
            PIPE_BUFFER_USAGE_CPU_READ_WRITE;
    pb->size = nouveau_bo(pb)->size;
    
    return pb;
}

static void
arosmesa_nouveau_cleanup( struct pipe_screen * screen )
{
    if (screen)
    {
        /* This destroys the winsys and closes the device */
        screen->destroy(screen);
    }
}

static struct pipe_surface *
arosmesa_nouveau_get_screen_surface(struct pipe_screen * screen, int width, int height, int bpp)
{
    struct pipe_surface *surface = NULL;
    struct pipe_texture *texture = NULL;
    struct pipe_texture templat;
    struct pipe_buffer *buf = NULL;
    unsigned pitch = width * bpp / 8;

    buf = nouveau_drm_pb_from_handle(screen, 1 /* driver makes sure object with ID 1 is visible framebuffer */);
    if (!buf)
        return NULL;

    memset(&templat, 0, sizeof(templat));
    templat.tex_usage = PIPE_TEXTURE_USAGE_PRIMARY |
                        NOUVEAU_TEXTURE_USAGE_LINEAR;
    templat.target = PIPE_TEXTURE_2D;
    templat.last_level = 0;
    templat.depth0 = 1;
    switch(bpp)
    {
        case(16):
            templat.format = PIPE_FORMAT_R5G6B5_UNORM;
            break;
        default:
            /* FIXME: should fail for anything other than 32bit */
            templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
            break;
    }
    templat.width0 = width;
    templat.height0 = height;

    texture = screen->texture_blanket(screen,
                                        &templat,
                                        &pitch,
                                        buf);

    /* we don't need the buffer from this point on */
    pipe_buffer_reference(&buf, NULL);

    if (!texture)
        return NULL;

    surface = screen->get_tex_surface(screen, texture, 0, 0, 0,
                                        PIPE_BUFFER_USAGE_GPU_READ |
                                        PIPE_BUFFER_USAGE_GPU_WRITE);

    /* we don't need the texture from this point on */
    pipe_texture_reference(&texture, NULL);

    return surface;
}

static void
arosmesa_nouveau_query_depth_stencil(int color, int * depth, int * stencil)
{
    (*depth)    = 24;
    (*stencil)  = 8;
}

struct arosmesa_driver arosmesa_nouveau_driver = 
{
    .create_pipe_screen = arosmesa_create_nouveau_screen,
    .create_pipe_context = arosmesa_create_nouveau_context,
    .display_surface = arosmesa_nouveau_display_surface,
    .get_screen_surface = arosmesa_nouveau_get_screen_surface,
    .cleanup = arosmesa_nouveau_cleanup,
    .query_depth_stencil = arosmesa_nouveau_query_depth_stencil,
};



