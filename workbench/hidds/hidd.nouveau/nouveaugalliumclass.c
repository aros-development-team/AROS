/*
    Copyright 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "nouveau_intern.h"

#include <aros/debug.h>
#include <proto/oop.h>

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "nouveau/nouveau_winsys.h"
#include "nv50/nv50_resource.h"
#include "nvfx/nvfx_resource.h"
#include "nvc0/nvc0_resource.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

struct HiddNouveauWinSys 
{
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
                              struct pipe_resource *resource,
                              unsigned level, unsigned layer,
                              void *winsys_drawable_handle  )
{
    /* No Op */
}

static VOID
HIDDNouveauDestroyWinSys(struct pipe_winsys *ws)
{
    struct HiddNouveauWinSys *nvws = HiddNouveauWinSys(ws);

    FREE(nvws);
}

/* Wraps the nouveau_bo from resource into 2D bitmap class data */
static BOOL
HIDDNouveauWrapResource(struct CardData * carddata, struct pipe_resource * resource,
    struct HIDDNouveauBitMapData * bmdata)
{
    struct nouveau_bo * bo = NULL;
    ULONG pitch = 0; ULONG depth = 0;

    /* Get buffer object and pitch */
    switch(carddata->architecture)
    {
        case NV_ARCH_30:
        case NV_ARCH_40:
            bo = nvfx_resource(resource)->bo;
            pitch = ((struct nvfx_miptree *)resource)->linear_pitch;
            break;
        case NV_ARCH_50:
            bo = nv50_miptree(resource)->base.bo;
            pitch = nv50_miptree(resource)->level[0].pitch;
            break;
        case NV_ARCH_C0:
            bo = nvc0_miptree(resource)->base.bo;
            pitch = nvc0_miptree(resource)->level[0].pitch;
            break;
    }
    
    if ((bo == NULL) || (pitch == 0))
        return FALSE;

    switch(resource->format)
    {
    case PIPE_FORMAT_B8G8R8A8_UNORM:
    case PIPE_FORMAT_A8R8G8B8_UNORM:
        /* For purpose of blitting render buffer to screen, 32-bit render
           buffer is treated as 24-bit surface. This is needed so that checks
           (src->depth == dst->depth) pass. */
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
    
    if (depth == 0)
        return FALSE;
    
    /* Set all fields */
    bmdata->bo = bo;
    bmdata->width = resource->width0;
    bmdata->height = resource->height0;
    bmdata->depth = depth;
    if (bmdata->depth == 16)
        bmdata->bytesperpixel = 2;
    else
        bmdata->bytesperpixel = 4;
    bmdata->pitch = pitch;
    bmdata->fbid = 0; /* Default value */
    InitSemaphore(&bmdata->semaphore);

    return TRUE;
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
    case 0x40:
    case 0x60:
        init = nvfx_screen_create;
        break;
    case 0x50:
    case 0x80:
    case 0x90:
    case 0xa0:
        init = nv50_screen_create;
        break;
    case 0xc0:
        init = nvc0_screen_create;
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

VOID METHOD(NouveauGallium, Hidd_Gallium, DisplayResource)
{
    struct CardData * carddata = &(SD(cl)->carddata);
    struct HIDDNouveauBitMapData srcdata;
    OOP_Object * bm = HIDD_BM_OBJ(msg->bitmap);
    struct HIDDNouveauBitMapData * dstdata;

    if (!IS_NOUVEAU_BM_CLASS(OOP_OCLASS(bm))) /* Check if bitmap is really nouveau bitmap */
        return;

    dstdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);

    if (!HIDDNouveauWrapResource(carddata, msg->resource, &srcdata))
        return;

    /* srcdata does not require a lock, because it's a local object that is
       access only by one task at a time */
    LOCK_BITMAP_BM(dstdata)
    /* XXX HACK XXX */
    /* HACK: there seems to something wrong with blitting method. Without this
    mapping, the blitting is jittering with high frame frames. Probably some
    flush is missing somewhere and doing a mapping executes this missing flush 
    */
    MAP_BUFFER_BM(dstdata)
    /* XXX HACK XXX */
    UNMAP_BUFFER_BM(dstdata)

    switch(carddata->architecture)
    {
    case NV_ARCH_30:
    case NV_ARCH_40:
        HIDDNouveauNV04CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
        break;
    case NV_ARCH_50:
        HIDDNouveauNV50CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
        break;    
    case NV_ARCH_C0:
        HIDDNouveauNVC0CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
    default:
        /* TODO: Report error */
        break;
    }


    UNLOCK_BITMAP_BM(dstdata)
}
