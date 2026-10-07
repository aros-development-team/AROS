/*
    Copyright 2015-2026, The AROS Development Team. All rights reserved.

    Desc: The 3D side: Mesa's radeonsi gallium driver over its stock
          amdgpu winsys, which talks to the in-process amdgpu driver
          through the libdrm shim.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <utility/tagitem.h>

#include <cybergraphx/cybergraphics.h>

#include <hidd/gallium.h>
#include <gallium/gallium.h>

#include "amdgpu_intern.h"

#include "pipe/p_context.h"      // For struct pipe_context
#include "pipe/p_screen.h"
#include "pipe/p_state.h"        // For struct pipe_transfer
#include "util/u_inlines.h"      // For pipe_texture_map()
#include "frontend/winsys_handle.h"
#include "util/os_time.h"

#ifdef GALLIUM_RADEONSI
#include "radeonsi/si_public.h"
#include "util/driconf.h"

static const driOptionDescription amdgpu_driconf[] = {
#include "pipe-loader/driinfo_gallium.h"
#include "radeonsi/driinfo_radeonsi.h"
};
#endif

#include <xf86drm.h>


// ****************************************************************************
//                      Gallium Hidd Methods
// ****************************************************************************

OOP_Object *METHOD(GalliumAmdgpu, Root, New)
{
    IPTR interfaceVers;

    D(bug("[Amdgpu:Gallium] %s()\n", __func__);)

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        struct HIDDGalliumAmdgpuData * data = OOP_INST_DATA(cl, o);

        memset(data, 0, sizeof(struct HIDDGalliumAmdgpuData));
        data->fd = -1;
    }

    return o;
}

VOID METHOD(GalliumAmdgpu, Root, Dispose)
{
    struct HIDDGalliumAmdgpuData * data = OOP_INST_DATA(cl, o);

    D(bug("[Amdgpu:Gallium] %s()\n", __func__);)

    if (data->option_cache.info)
        driDestroyOptionCache(&data->option_cache);
    if (data->option_info.info)
        driDestroyOptionInfo(&data->option_info);

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(GalliumAmdgpu, Root, Get)
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

APTR METHOD(GalliumAmdgpu, Hidd_Gallium, CreatePipeScreen)
{
    struct HIDDGalliumAmdgpuData * data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = NULL;

    D(bug("[Amdgpu:Gallium] %s()\n", __func__);)

    if (data->screen)
        return data->screen;

    data->fd = drmOpen(NULL, NULL);
    if (data->fd < 0)
    {
        bug("[Amdgpu:Gallium] %s: no drm file (%d)\n", __func__, data->fd);
        return NULL;
    }

#ifdef GALLIUM_RADEONSI
    {
        struct pipe_screen_config config = { 0 };

        if (!data->option_info.info)
            driParseOptionInfo(&data->option_info, amdgpu_driconf, ARRAY_SIZE(amdgpu_driconf));
        config.options_info = &data->option_info;
        config.options = &data->option_cache;
        screen = radeonsi_screen_create(data->fd, &config);
    }
#endif
    D(bug("[Amdgpu:Gallium] %s: screen @ 0x%p\n", __func__, screen));
    if (!screen)
    {
        drmClose(data->fd);
        data->fd = -1;
        return NULL;
    }

    data->screen = screen;
    data->pipe = screen->context_create(screen, data, 0);
    D(bug("[Amdgpu:Gallium] %s: pipe @ 0x%p\n", __func__, data->pipe));

    return screen;
}

VOID METHOD(GalliumAmdgpu, Hidd_Gallium, DestroyPipeScreen)
{
    struct HIDDGalliumAmdgpuData * data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

    D(bug("[Amdgpu:Gallium] %s(0x%p)\n", __func__, screen);)

    if (!screen || screen != data->screen)
        return;

    pipe_resource_reference(&data->scanout, NULL);
    if (data->pipe)
    {
        data->pipe->destroy(data->pipe);
        data->pipe = NULL;
    }
    screen->destroy(screen);
    data->screen = NULL;

    if (data->fd >= 0)
    {
        drmClose(data->fd);
        data->fd = -1;
    }
}

static struct pipe_resource *amdgpu_scanout_resource(OOP_Class *cl, struct HIDDGalliumAmdgpuData *data,
                                                     struct BitmapData *bmdata)
{
    struct Amdgpu_FB *fb = &bmdata->fb;
    struct drm_gem_flink flink;
    struct winsys_handle wh;
    struct pipe_resource templ;

    if (data->scanout && data->scanout_handle == fb->handle && data->scanout_map == fb->map)
        return data->scanout;
    pipe_resource_reference(&data->scanout, NULL);

    memset(&flink, 0, sizeof(flink));
    flink.handle = fb->handle;
    if (drmIoctl(XSD(cl)->kms.fd, DRM_IOCTL_GEM_FLINK, &flink) != 0)
        return NULL;

    memset(&wh, 0, sizeof(wh));
    wh.type = WINSYS_HANDLE_TYPE_SHARED;
    wh.handle = flink.name;
    wh.stride = fb->pitch;
    wh.format = PIPE_FORMAT_B8G8R8X8_UNORM;

    memset(&templ, 0, sizeof(templ));
    templ.target = PIPE_TEXTURE_2D;
    templ.format = PIPE_FORMAT_B8G8R8X8_UNORM;
    templ.width0 = fb->width;
    templ.height0 = fb->height;
    templ.depth0 = 1;
    templ.array_size = 1;
    templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SCANOUT | PIPE_BIND_SHARED;

    data->scanout = data->screen->resource_from_handle(data->screen, &templ, &wh,
                                                       PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if (data->scanout)
    {
        data->scanout_handle = fb->handle;
        data->scanout_map = fb->map;
    }
    return data->scanout;
}

static BOOL amdgpu_display_gpu(OOP_Class *cl, struct HIDDGalliumAmdgpuData *data,
                               struct pHidd_Gallium_DisplayResource *msg)
{
    OOP_Object *bm = HIDD_BM_OBJ(msg->bitmap);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_resource *dst;
    struct pipe_fence_handle *fence = NULL;
    struct pipe_blit_info blit;
    struct BitmapData *bmdata;

    if (!bm || OOP_OCLASS(bm) != XSD(cl)->amdgpuonbmclass)
        return FALSE;
    bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    if (!bmdata->fb.handle)
        return FALSE;

    ObtainSemaphore(&bmdata->bmsem);
    dst = amdgpu_scanout_resource(cl, data, bmdata);
    if (dst)
    {
        memset(&blit, 0, sizeof(blit));
        blit.src.resource = res;
        blit.src.format = res->format;
        blit.src.box.x = msg->srcx;
        blit.src.box.y = msg->srcy;
        blit.src.box.width = msg->width;
        blit.src.box.height = msg->height;
        blit.src.box.depth = 1;
        blit.dst.resource = dst;
        blit.dst.format = dst->format;
        blit.dst.box.x = msg->dstx;
        blit.dst.box.y = msg->dsty;
        blit.dst.box.width = msg->width;
        blit.dst.box.height = msg->height;
        blit.dst.box.depth = 1;
        blit.mask = PIPE_MASK_RGBA;
        blit.filter = PIPE_TEX_FILTER_NEAREST;

        data->pipe->blit(data->pipe, &blit);
        data->pipe->flush(data->pipe, &fence, 0);
        if (fence)
        {
            data->screen->fence_finish(data->screen, NULL, fence, OS_TIMEOUT_INFINITE);
            data->screen->fence_reference(data->screen, &fence, NULL);
        }
    }
    ReleaseSemaphore(&bmdata->bmsem);

    return dst != NULL;
}

VOID METHOD(GalliumAmdgpu, Hidd_Gallium, DisplayResource)
{
    struct HIDDGalliumAmdgpuData * data = OOP_INST_DATA(cl, o);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_context *pipe = data->pipe;
    struct pipe_transfer *transfer = NULL;
    uint8_t *mapped;
    struct RastPort *rp;

    D(
        bug("[Amdgpu:Gallium] %s: pipe @ %p\n", __func__, pipe);
        bug("[Amdgpu:Gallium] %s: resource @ %p\n", __func__, res);
        bug("[Amdgpu:Gallium] %s: bitmap @ %p (%u,%u -> %u,%u)\n", __func__,
            msg->bitmap, msg->srcx, msg->srcy, msg->srcx + msg->width, msg->srcy + msg->height);
    )

    if (!pipe || !res || !msg->bitmap)
        return;

    if (amdgpu_display_gpu(cl, data, msg))
        return;

    // Map the resource for CPU read
    mapped = pipe_texture_map(pipe, res, 0, 0, PIPE_MAP_READ,
                              msg->srcx, msg->srcy, msg->width, msg->height,
                              &transfer);
    if (!mapped) {
        bug("[Amdgpu:Gallium] texture_map failed\n");
        return;
    }

    D(bug("[Amdgpu:Gallium] %s: resource mapped @ 0x%p\n", __func__, mapped));
    rp = CreateRastPort();
    if (rp) {
        rp->BitMap = msg->bitmap;

        WritePixelArray(
            mapped,
            0, 0,
            transfer->stride,               // Row stride
            rp,
            msg->dstx, msg->dsty,
            msg->width, msg->height,
            AROS_PIXFMT);

        FreeRastPort(rp);
        D(bug("[Amdgpu:Gallium] %s: resource output\n", __func__);)
    }

    // Unmap
    pipe_texture_unmap(pipe, transfer);
    D(bug("[Amdgpu:Gallium] %s: done\n", __func__);)
}
