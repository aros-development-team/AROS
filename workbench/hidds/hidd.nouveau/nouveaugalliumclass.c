/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "nouveau/nouveau_screen.h"
#include "nouveau/nouveau_drmif.h"
#include "nouveau/nouveau_winsys.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

struct HiddNouveauWinSys {
    struct pipe_winsys base;

    struct pipe_screen *pscreen;
};

static INLINE struct HiddNouveauWinSys *
HiddNouveauWinSys(struct pipe_winsys *ws)
{
    return (struct HiddNouveauWinSys *)ws;
}

static void 
HiddNouveauFlushFrontBuffer( struct pipe_screen *screen,
                            struct pipe_surface *surf,
                            void *context_private )
{
    /* No Op */
}

static void
HiddNouveauDestroyWinSys(struct pipe_winsys *ws)
{
    struct HiddNouveauWinSys *nvws = HiddNouveauWinSys(ws);

    FREE(nvws);
}

static struct pipe_buffer *
HiddNouveauPipeBufferFromBuffer(struct pipe_screen *pscreen, struct nouveau_bo * bo)
{
    struct pipe_buffer *pb;

    pb = CALLOC(1, sizeof(struct pipe_buffer) + sizeof(struct nouveau_bo*));
    if (!pb)
        return NULL;

    /* Reference the buffer */
    nouveau_bo_ref(bo, (struct nouveau_bo **)(pb + 1));

    pipe_reference_init(&pb->reference, 1);
    pb->screen = pscreen;
    pb->alignment = 0;
    pb->usage = PIPE_BUFFER_USAGE_GPU_READ_WRITE |
            PIPE_BUFFER_USAGE_CPU_READ_WRITE;
    pb->size = nouveau_bo(pb)->size;
    
    return pb;
}

static struct pipe_surface *
HiddNouveauGetScreenSurface(struct pipe_screen * screen, struct HIDDNouveauBitMapData * screenbitmap)
{
    struct pipe_surface *surface = NULL;
    struct pipe_texture *texture = NULL;
    struct pipe_texture templat;
    struct pipe_buffer *buf = NULL;
    LONG width = screenbitmap->width;
    LONG height = screenbitmap->height;
    LONG bpp = screenbitmap->bytesperpixel * 8;
    unsigned pitch = screenbitmap->pitch;

    buf = HiddNouveauPipeBufferFromBuffer(screen, screenbitmap->bo);
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
            templat.format = PIPE_FORMAT_B5G6R5_UNORM;
            break;
        case(32):
            templat.format = PIPE_FORMAT_B8G8R8A8_UNORM;
            break;
        default:
            /* Fail */
            pipe_buffer_reference(&buf, NULL);
            return NULL;
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

/* METHODS */
OOP_Object *METHOD(NouveauGallium, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if(o)
    {
        /* Check if chipset support hardware 3D via gallium */
        if ((SD(cl)->carddata.dev == NULL) || (SD(cl)->carddata.dev->chipset < 0x30))
        {
            OOP_MethodID dispose_mid;
            dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg) & dispose_mid);        
            o = NULL;
        }
    }

    return o;
}

VOID METHOD(NouveauGallium, Root, Get)
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

APTR METHOD(NouveauGallium, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddNouveauWinSys *nvws;
    struct pipe_winsys *ws;
    struct nouveau_device *dev = SD(cl)->carddata.dev;
    struct pipe_screen *(*init)(struct pipe_winsys *,
                    struct nouveau_device *);

    switch (dev->chipset & 0xf0) 
    {
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
        D(bug("%s: unknown chipset nv%02x\n", __func__,
                 dev->chipset));
        return NULL;
    }


    nvws = CALLOC_STRUCT(HiddNouveauWinSys);
    if (!nvws) {
        nouveau_device_close(&dev);
        return NULL;
    }
    ws = &nvws->base;
    ws->destroy = HiddNouveauDestroyWinSys;

    nvws->pscreen = init(ws, dev);
    if (!nvws->pscreen) {
        ws->destroy(ws);
        return NULL;
    }
    
    nvws->pscreen->flush_frontbuffer = HiddNouveauFlushFrontBuffer;

    return nvws->pscreen;
}

VOID METHOD(NouveauGallium, Hidd_Gallium, DisplaySurface)
{
    /* unmap screenbitmap */
    /* get screen surface */
    /* copy */
    /* release screen surface */
    /* map screenbitmap */
    
    struct pipe_context * pipe = (struct pipe_context *)msg->context;
    struct pipe_surface * surface = (struct pipe_surface *)msg->surface;
    struct pipe_surface * screensurface = NULL;
    
    LOCK_BITMAP_BM(SD(cl)->screenbitmap);
    
    nouveau_bo_unmap(SD(cl)->screenbitmap->bo);

    screensurface = HiddNouveauGetScreenSurface(pipe->screen, SD(cl)->screenbitmap);

    if (!screensurface)
    {
        nouveau_bo_map(SD(cl)->screenbitmap->bo, NOUVEAU_BO_RDWR);
        UNLOCK_BITMAP_BM(SD(cl)->screenbitmap);
        return;
    }
    
    pipe->surface_copy(pipe, screensurface, 
                msg->absx, 
                msg->absy, 
                surface, 
                msg->left, 
                msg->top, 
                msg->width, 
                msg->height);

    pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

    pipe_surface_reference(&screensurface, NULL);

    nouveau_bo_map(SD(cl)->screenbitmap->bo, NOUVEAU_BO_RDWR);
    
    UNLOCK_BITMAP_BM(SD(cl)->screenbitmap);
}

VOID METHOD(NouveauGallium, Hidd_Gallium, DestroyPipeScreen)
{
    struct pipe_screen * screen = (struct pipe_screen *)msg->screen;

    if (screen)
        screen->destroy(screen);
}
