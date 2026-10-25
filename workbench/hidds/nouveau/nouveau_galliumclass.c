/*
    Copyright 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"

#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <gallium/gallium.h>

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "nouveau_screen.h"
#include "nouveau_winsys.h"
#include "nv50/nv50_resource.h"
#include "nv30/nv30_resource.h"
#include "frontend/winsys_handle.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->galliumAttrBase)

/* Wraps the nouveau_bo from resource into 2D bitmap class data */
static BOOL
HIDDNouveauWrapResource(struct CardData * carddata, struct pipe_resource * resource,
    struct HIDDNouveauBitMapData * bmdata)
{
    struct nouveau_bo * bo = NULL;
    ULONG pitch = 0; ULONG depth = 0;

    /* Get buffer object and pitch */
    switch(carddata->Architecture)
    {
        case NV_ARCH_30:
        case NV_ARCH_40:
            bo = nv30_miptree(resource)->base.bo;
            pitch = nv30_miptree(resource)->level[0].pitch;
            break;
        case NV_TESLA:
        case NV_FERMI:
        case NV_KEPLER:
        case NV_MAXWELL:
        case NV_PASCAL:
        case NV_VOLTA:
        case NV_TURING:
        case NV_AMPERE:
        case NV_HOPPER:
        case NV_ADA:
        case NV_BLACKWELL:
            bo = nv50_miptree(resource)->base.bo;
            pitch = nv50_miptree(resource)->level[0].pitch;
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
    bmdata->drawable.width = resource->width0;
    bmdata->drawable.height = resource->height0;
    bmdata->drawable.depth = bmdata->drawable.bitsPerPixel = depth;
    bmdata->drawable.pScreen = carddata;
    if (bmdata->drawable.depth == 16)
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
    IPTR interfaceVers;

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

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
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(NouveauGallium, Hidd_Gallium, CreatePipeScreen)
{
    struct HIDDGalliumNouveauData *data = OOP_INST_DATA(cl, o);
    struct nouveau_device *dev = SD(cl)->carddata.dev;
    struct nouveau_screen *(*init)(struct nouveau_device *);
    struct nouveau_screen *screen = NULL;

    switch (dev->chipset & 0xff0)
    {
    case 0x030:
    case 0x040:
    case 0x060:
        init = nv30_screen_create;
        break;
    case 0x050:
    case 0x080:
    case 0x090:
    case 0x0a0:
        init = nv50_screen_create;
        break;
    case 0x0c0:
    case 0x0d0:
    case 0x0e0:
    case 0x0f0:
    case 0x100:
    case 0x110:
    case 0x120:
    case 0x130:
    case 0x140:
    case 0x160:
    case 0x170:
    case 0x190:
        init = nvc0_screen_create;
        break;
    default:
        D(bug("%s: unknown chipset nv%02x\n", __func__,
                 dev->chipset));
        return NULL;
    }

    LOCK_ENGINE
    screen = init(dev);
    if (!screen) {
        UNLOCK_ENGINE
        return NULL;
    }
    if (!screen->base.context_create) {
        screen->base.destroy(&screen->base);
        UNLOCK_ENGINE
        return NULL;
    }
    /* what the drm winsys would have done for us */
    screen->initialized = true;
    UNLOCK_ENGINE

    return screen;
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

    LOCK_ENGINE

    /* srcdata does not require a lock, because it's a local object that is
       access only by one task at a time */
    LOCK_BITMAP_BM(dstdata)
    /* XXX HACK XXX */
    /* HACK: there seems to something wrong with blitting method. Without this
    mapping, the blitting is jittering with high frame frames. Probably some
    flush is missing somewhere and doing a mapping executes this missing flush 
    */
    // MAP_BUFFER_BM(dstdata)
    // /* XXX HACK XXX */
    // UNMAP_BUFFER_BM(dstdata)

    switch(carddata->Architecture)
    {
    case NV_ARCH_30:
    case NV_ARCH_40:
        HIDDNouveauNV04CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
        break;
    case NV_TESLA:
        HIDDNouveauNV50CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
        break;
    case NV_FERMI:
    case NV_KEPLER:
    case NV_MAXWELL:
    case NV_PASCAL:
    case NV_VOLTA:
    case NV_TURING:
    case NV_AMPERE:
    case NV_HOPPER:
    case NV_ADA:
    case NV_BLACKWELL:
        HIDDNouveauNVC0CopySameFormat(carddata, &srcdata, dstdata, 
            msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height, 
            0x03 /* vHidd_GC_DrawMode_Copy */);
        break;
    default:
        /* TODO: Report error */
        break;
    }

nouveau_bo_wait(dstdata->bo, NOUVEAU_BO_RD, carddata->client);

    UNLOCK_BITMAP_BM(dstdata)

    UNLOCK_ENGINE
}

/*
 * Alias buffer `buffer` of a nouveau scan-out bitmap as a pipe_resource of
 * the gallium screen `screen` (3D scan-out, plan 3.7 / 4.6). The gallium
 * screen and the hidd share one nouveau_device, so the bo is handed over
 * through its GEM name: nouveau_screen_bo_from_handle() finds the same
 * nouveau_bo in the device's list and takes a reference, the miptree
 * takes the bitmap's pitch as its level-0 pitch and the bo's (linear)
 * config as its layout. The 2D/3D engines render linear targets by pitch
 * (bo memtype 0), which is exactly what the window plane
 * scans out and what the compositor's CopyBox reads.
 */
APTR METHOD(NouveauGallium, Hidd_Gallium, ResourceFromBitMap)
{
    struct pipe_screen *pscreen = (struct pipe_screen *)msg->screen;
    OOP_Object *bm = msg->bitmap;
    struct HIDDNouveauBitMapData *bmdata;
    struct nouveau_bo *bo;
    struct winsys_handle whandle;
    struct pipe_resource templ;
    struct pipe_resource *res;
    enum pipe_format format;
    uint32_t name = 0;

    if (!pscreen || !bm || !pscreen->resource_from_handle)
        return NULL;
    if (!IS_NOUVEAU_BM_CLASS(OOP_OCLASS(bm)))
        return NULL;

    bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    if (!bmdata->nbufs || msg->buffer >= bmdata->nbufs || bmdata->videoformat)
        return NULL;

    bo = bmdata->buf[msg->buffer].bo;
    if (!bo || !bmdata->pitch)
        return NULL;

    /* Only linear buffers: the plane and the compositor read them by pitch
     * (memtype 0: nv50 and nvc0 share the field) */
    if (bo->config.nv50.memtype != 0)
    {
        D(bug("[NouveauGallium] %s: buffer %lu of %p is tiled (memtype %x), refused\n",
              __func__, (unsigned long)msg->buffer, bm, bo->config.nv50.memtype));
        return NULL;
    }

    switch (bmdata->stdfmt)
    {
    case vHidd_StdPixFmt_BGRA32:
        format = PIPE_FORMAT_B8G8R8A8_UNORM;
        break;
    case vHidd_StdPixFmt_BGR032:
        format = PIPE_FORMAT_B8G8R8X8_UNORM;
        break;
    case vHidd_StdPixFmt_RGB16_LE:
        format = PIPE_FORMAT_B5G6R5_UNORM;
        break;
    default:
        D(bug("[NouveauGallium] %s: std pixfmt %lu has no pipe format\n", __func__,
              (unsigned long)bmdata->stdfmt));
        return NULL;
    }

    if (nouveau_bo_name_get(bo, &name) || !name)
    {
        D(bug("[NouveauGallium] %s: no GEM name for bo %p\n", __func__, bo));
        return NULL;
    }

    memset(&whandle, 0, sizeof(whandle));
    whandle.type   = WINSYS_HANDLE_TYPE_SHARED;
    whandle.handle = name;
    whandle.stride = bmdata->pitch;
    whandle.offset = 0;

    memset(&templ, 0, sizeof(templ));
    templ.target     = pscreen->caps.npot_textures ? PIPE_TEXTURE_2D : PIPE_TEXTURE_RECT;
    templ.format     = format;
    templ.width0     = bmdata->drawable.width;
    templ.height0    = bmdata->drawable.height;
    templ.depth0     = 1;
    templ.array_size = 1;
    templ.last_level = 0;
    templ.nr_samples = 0;
    templ.nr_storage_samples = 0;
    templ.bind       = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW
                     | PIPE_BIND_SCANOUT | PIPE_BIND_LINEAR | PIPE_BIND_SHARED;
    templ.usage      = PIPE_USAGE_DEFAULT;

    res = pscreen->resource_from_handle(pscreen, &templ, &whandle,
                                        PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);

    D(bug("[NouveauGallium] %s: bitmap %p buffer %lu bo %p name %lu pitch %lu -> resource %p\n",
          __func__, bm, (unsigned long)msg->buffer, bo, (unsigned long)name,
          (unsigned long)bmdata->pitch, res));

    return res;
}
