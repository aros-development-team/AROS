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
    struct HIDDT_WinSys base;

    struct pipe_screen *pscreen;
};

static INLINE struct HiddNouveauWinSys *
HiddNouveauWinSys(struct pipe_winsys *ws)
{
    return (struct HiddNouveauWinSys *)ws;
}

static VOID
HIDDNouveauFlushFrontBuffer( struct pipe_screen *screen,
                            struct pipe_surface *surf,
                            void *context_private )
{
    /* No Op */
}

static VOID
HIDDNouveauDestroyWinSys(struct pipe_winsys *ws)
{
    struct HiddNouveauWinSys *nvws = HiddNouveauWinSys(ws);

    FREE(nvws);
}

#include "nv40/nv40_state.h"
#include "nv30/nv30_state.h"
#include "nouveau/nv04_surface_2d.h"
#include "nv50/nv50_context.h"

/* Wraps the nouveau_bo from surface into 2D bitmap class data */
static struct HIDDNouveauBitMapData * 
HIDDNouveauWrapSurface(struct CardData * carddata, struct pipe_surface * surface)
{
    struct HIDDNouveauBitMapData * bmdata = NULL;
    struct nouveau_bo * bo = NULL;
    ULONG pitch = 0; ULONG depth = 0;
    
    /* Get buffer object and pitch */
    switch(carddata->architecture)
    {
        case NV_ARCH_30:
            bo = ((struct nv30_miptree *)surface->texture)->bo;
            pitch = ((struct nv04_surface *)surface)->pitch;
            break;
        case NV_ARCH_40:
            bo = ((struct nv40_miptree *)surface->texture)->bo;
            pitch = ((struct nv04_surface *)surface)->pitch;
            break;
        case NV_ARCH_50:
            bo = nv50_miptree(surface->texture)->base.bo;
            pitch = nv50_miptree(surface->texture)->level[surface->level].pitch;
            break;
    }
    
    if ((NULL == bo) || (0 == pitch))
        return NULL;

    switch(surface->format)
    {
    case PIPE_FORMAT_B8G8R8A8_UNORM:
    case PIPE_FORMAT_A8R8G8B8_UNORM:
        depth = 32;
        break;
    case PIPE_FORMAT_B8G8R8X8_UNORM:
    case PIPE_FORMAT_X8R8G8B8_UNORM:
        depth = 24;
        break;
    case PIPE_FORMAT_B5G5R5A1_UNORM:
    case PIPE_FORMAT_B4G4R4A4_UNORM:
    case PIPE_FORMAT_B5G6R5_UNORM:
        depth = 16;
        break;
    default:
        depth = 0;
        break;
    }
    
    if (0 == depth)
        return NULL;
    
    /* Create wrapper object */
    bmdata = AllocVec(sizeof(struct HIDDNouveauBitMapData), MEMF_ANY | MEMF_CLEAR);

    /* Set all fields */
    bmdata->bo = bo;
    bmdata->width = surface->width;
    bmdata->height = surface->height;
    bmdata->depth = depth;
    if (bmdata->depth <= 8)
        bmdata->bytesperpixel = 1;
    else if (bmdata->depth <= 16)
        bmdata->bytesperpixel = 2;
    else
        bmdata->bytesperpixel = 4;
    bmdata->pitch = pitch;
    bmdata->fbid = 0; /* Default value */
    InitSemaphore(&bmdata->semaphore);

    return bmdata;
}

static VOID 
HIDDNouveauReleaseWrap(struct HIDDNouveauBitMapData * bmdata)
{
    FreeVec(bmdata);
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
        return NULL;
    }
    ws = &nvws->base.base;
    ws->destroy = HIDDNouveauDestroyWinSys;

    nvws->pscreen = init(ws, dev);
    if (!nvws->pscreen) {
        ws->destroy(ws);
        return NULL;
    }
    
    nvws->pscreen->flush_frontbuffer = HIDDNouveauFlushFrontBuffer;
    
    /* Preserve pointer to HIDD driver */
    nvws->base.driver = o;
    
    return nvws->pscreen;
}

VOID METHOD(NouveauGallium, Hidd_Gallium, DisplaySurface)
{
    struct pipe_surface * surface = (struct pipe_surface *)msg->surface;
    struct CardData * carddata = &(SD(cl)->carddata);
    struct HIDDNouveauBitMapData * srcdata = HIDDNouveauWrapSurface(carddata, surface);
    
    if (!srcdata)
        return;
    
    LOCK_MULTI_BITMAP
    LOCK_BITMAP_BM(srcdata)
    LOCK_BITMAP_BM(SD(cl)->screenbitmap)
    UNLOCK_MULTI_BITMAP

    if (carddata->architecture < NV_ARCH_50)
    {
        HIDDNouveauNV04CopySameFormat(carddata, srcdata, SD(cl)->screenbitmap, 
            msg->left, msg->top, msg->absx, msg->absy, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
    }
    else
    {
        HIDDNouveauNV50CopySameFormat(carddata, srcdata, SD(cl)->screenbitmap, 
            msg->left, msg->top, msg->absx, msg->absy, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
    }

    UNLOCK_BITMAP_BM(SD(cl)->screenbitmap)
    UNLOCK_BITMAP_BM(srcdata)

    HIDDNouveauReleaseWrap(srcdata);
}

