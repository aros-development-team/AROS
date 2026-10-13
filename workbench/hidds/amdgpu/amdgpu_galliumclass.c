/*
    Copyright 2015-2026, The AROS Development Team. All rights reserved.

    Desc: The 3D side: Mesa's radeonsi gallium driver over its stock
          amdgpu winsys, which talks to the in-process amdgpu driver
          through the libdrm shim.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
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
#include <stdlib.h>

int amdgpu_aros_wait_vblank(unsigned int crtc_id);
static void amdgpu_presenter_stop(struct HIDDGalliumAmdgpuData *data);


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
        char value[8];

        memset(data, 0, sizeof(struct HIDDGalliumAmdgpuData));
        data->fd = -1;
        data->swap_interval = AMDGPU_SWAP_MAILBOX;
        data->cl = cl;
        InitSemaphore(&data->mbox_lock);
        if (GetVar("AMDGPU_SWAP_INTERVAL", value, sizeof(value), 0) > 0)
            data->swap_interval = strtoul(value, NULL, 10);
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

    amdgpu_presenter_stop(data);
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

static void amdgpu_blit(struct pipe_context *pipe, struct pipe_resource *src, const struct pipe_box *box,
                        struct pipe_resource *dst, LONG dstx, LONG dsty)
{
    struct pipe_blit_info blit;

    memset(&blit, 0, sizeof(blit));
    blit.src.resource = src;
    blit.src.format = src->format;
    blit.src.box = *box;
    blit.dst.resource = dst;
    blit.dst.format = dst->format;
    blit.dst.box.x = dstx;
    blit.dst.box.y = dsty;
    blit.dst.box.width = box->width;
    blit.dst.box.height = box->height;
    blit.dst.box.depth = 1;
    blit.mask = PIPE_MASK_RGBA;
    blit.filter = PIPE_TEX_FILTER_NEAREST;
    pipe->blit(pipe, &blit);
}

static void amdgpu_finish(struct pipe_screen *screen, struct pipe_context *pipe)
{
    struct pipe_fence_handle *fence = NULL;

    pipe->flush(pipe, &fence, 0);
    if (fence)
    {
        screen->fence_finish(screen, NULL, fence, OS_TIMEOUT_INFINITE);
        screen->fence_reference(screen, &fence, NULL);
    }
}

/* Shows the newest frame of the mailbox once per vblank. */
static void amdgpu_presenter(void)
{
    struct HIDDGalliumAmdgpuData *data = FindTask(NULL)->tc_UserData;
    OOP_Class *cl = data->cl;
    BYTE sigbit = AllocSignal(-1);

    data->presenter_sigmask = sigbit >= 0 ? 1UL << sigbit : 0;
    Signal(data->presenter_parent, data->presenter_ack);
    if (sigbit < 0)
        return;

    while (!data->presenter_stop)
    {
        struct BitmapData *bm;
        struct pipe_resource *dst;
        ULONG i;

        if (!data->mbox_dirty)
        {
            Wait(data->presenter_sigmask);
            continue;
        }
        bm = data->mbox_bm;
        if (bm && XSD(cl)->kms.curfb == &bm->fb)
            amdgpu_aros_wait_vblank(XSD(cl)->kms.crtc_id);

        ObtainSemaphore(&data->mbox_lock);
        bm = data->mbox_bm;
        if (data->mbox_dirty && bm && data->mbox)
        {
            if (data->mbox_fence)
                data->screen->fence_finish(data->screen, NULL, data->mbox_fence, OS_TIMEOUT_INFINITE);
            ObtainSemaphore(&bm->bmsem);
            Amdgpu_2D_Sync();
            dst = amdgpu_scanout_resource(cl, data, bm);
            if (dst)
            {
                for (i = 0; i < data->mbox_count; i++)
                    amdgpu_blit(data->present_pipe, data->mbox, &data->mbox_src[i], dst,
                                data->mbox_dstx[i], data->mbox_dsty[i]);
                amdgpu_finish(data->screen, data->present_pipe);
            }
            ReleaseSemaphore(&bm->bmsem);
        }
        data->mbox_dirty = FALSE;
        ReleaseSemaphore(&data->mbox_lock);
    }
    FreeSignal(sigbit);
    data->presenter_done = TRUE;
    Signal(data->presenter_parent, data->presenter_ack);
}

static BOOL amdgpu_presenter_start(struct HIDDGalliumAmdgpuData *data)
{
    BYTE ack;

    if (data->presenter)
        return TRUE;
    data->present_pipe = data->screen->context_create(data->screen, NULL, 0);
    if (!data->present_pipe)
        return FALSE;
    ack = AllocSignal(-1);
    if (ack < 0)
    {
        data->present_pipe->destroy(data->present_pipe);
        data->present_pipe = NULL;
        return FALSE;
    }
    data->presenter_ack = 1UL << ack;
    data->presenter_parent = FindTask(NULL);
    data->presenter_stop = FALSE;
    data->presenter_done = FALSE;
    data->presenter = (struct Task *)CreateNewProcTags(NP_Name, (IPTR)"Amdgpu GL Present",
                                                       NP_Priority, 22,
                                                       NP_Affinity, TASKAFFINITY_ANY,
                                                       NP_Entry, (IPTR)amdgpu_presenter,
                                                       NP_UserData, (IPTR)data,
                                                       NP_StackSize, 256 * 1024,
                                                       TAG_DONE);
    if (!data->presenter)
    {
        FreeSignal(ack);
        data->present_pipe->destroy(data->present_pipe);
        data->present_pipe = NULL;
        return FALSE;
    }
    Wait(data->presenter_ack);
    FreeSignal(ack);
    return data->presenter_sigmask != 0;
}

static void amdgpu_presenter_stop(struct HIDDGalliumAmdgpuData *data)
{
    BYTE ack;

    if (!data->presenter)
        return;
    ack = AllocSignal(-1);
    data->presenter_ack = ack >= 0 ? 1UL << ack : 0;
    data->presenter_parent = FindTask(NULL);
    data->presenter_stop = TRUE;
    Signal(data->presenter, data->presenter_sigmask);
    if (ack >= 0)
    {
        Wait(data->presenter_ack);
        FreeSignal(ack);
    }
    while (!data->presenter_done)
        Delay(1);
    data->presenter = NULL;
    data->present_pipe->destroy(data->present_pipe);
    data->present_pipe = NULL;
    pipe_resource_reference(&data->mbox, NULL);
    if (data->mbox_fence)
        data->screen->fence_reference(data->screen, &data->mbox_fence, NULL);
}

/* Queue a frame region for the presenter; a region overlapping one already
   queued starts a new frame. */
static BOOL amdgpu_display_mailbox(struct HIDDGalliumAmdgpuData *data, struct BitmapData *bmdata,
                                   struct pipe_resource *res, struct pHidd_Gallium_DisplayResource *msg)
{
    struct pipe_box box;
    ULONG i;

    if (!amdgpu_presenter_start(data))
        return FALSE;

    memset(&box, 0, sizeof(box));
    box.x = msg->srcx;
    box.y = msg->srcy;
    box.width = msg->width;
    box.height = msg->height;
    box.depth = 1;

    ObtainSemaphore(&data->mbox_lock);
    if (!data->mbox || data->mbox->width0 != res->width0 || data->mbox->height0 != res->height0 ||
        data->mbox->format != res->format)
    {
        struct pipe_resource templ = *res;

        pipe_resource_reference(&data->mbox, NULL);
        templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
        templ.usage = PIPE_USAGE_DEFAULT;
        templ.nr_samples = templ.nr_storage_samples = 0;
        data->mbox = data->screen->resource_create(data->screen, &templ);
        data->mbox_count = 0;
    }
    if (!data->mbox)
    {
        ReleaseSemaphore(&data->mbox_lock);
        return FALSE;
    }
    if (data->mbox_bm != bmdata)
        data->mbox_count = 0;
    for (i = 0; i < data->mbox_count; i++)
    {
        const struct pipe_box *q = &data->mbox_src[i];

        if (box.x < q->x + q->width && q->x < box.x + box.width &&
            box.y < q->y + q->height && q->y < box.y + box.height)
        {
            data->mbox_count = 0;
            break;
        }
    }
    if (data->mbox_count < 32)
    {
        data->mbox_src[data->mbox_count] = box;
        data->mbox_dstx[data->mbox_count] = msg->dstx;
        data->mbox_dsty[data->mbox_count] = msg->dsty;
        data->mbox_count++;
    }
    data->pipe->resource_copy_region(data->pipe, data->mbox, 0, box.x, box.y, 0, res, 0, &box);
    if (data->mbox_fence)
        data->screen->fence_reference(data->screen, &data->mbox_fence, NULL);
    data->pipe->flush(data->pipe, &data->mbox_fence, 0);
    data->mbox_bm = bmdata;
    data->mbox_dirty = TRUE;
    ReleaseSemaphore(&data->mbox_lock);

    Signal(data->presenter, data->presenter_sigmask);
    return TRUE;
}

static BOOL amdgpu_display_gpu(OOP_Class *cl, struct HIDDGalliumAmdgpuData *data,
                               struct pHidd_Gallium_DisplayResource *msg)
{
    OOP_Object *bm = HIDD_BM_OBJ(msg->bitmap);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_resource *dst;
    struct BitmapData *bmdata;
    struct pipe_box box;

    if (!bm || OOP_OCLASS(bm) != XSD(cl)->amdgpuonbmclass)
        return FALSE;
    bmdata = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    if (!bmdata->fb.handle)
        return FALSE;

    if (data->swap_interval == AMDGPU_SWAP_MAILBOX)
        return amdgpu_display_mailbox(data, bmdata, res, msg);

    if (XSD(cl)->kms.curfb == &bmdata->fb)
    {
        ULONG n;

        for (n = 0; n < data->swap_interval; n++)
            amdgpu_aros_wait_vblank(XSD(cl)->kms.crtc_id);
    }

    memset(&box, 0, sizeof(box));
    box.x = msg->srcx;
    box.y = msg->srcy;
    box.width = msg->width;
    box.height = msg->height;
    box.depth = 1;

    ObtainSemaphore(&bmdata->bmsem);
    Amdgpu_2D_Sync();
    dst = amdgpu_scanout_resource(cl, data, bmdata);
    if (dst)
    {
        amdgpu_blit(data->pipe, res, &box, dst, msg->dstx, msg->dsty);
        amdgpu_finish(data->screen, data->pipe);
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
