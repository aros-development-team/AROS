/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: 2D operations on the screen bitmap done by the GPU through a
          radeonsi context of the hidd's own.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <exec/ports.h>

#include <string.h>

#include "amdgpu_intern.h"
#include "amdgpu_2d.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/os_time.h"
#include "util/driconf.h"
#include "frontend/winsys_handle.h"
#include "radeonsi/si_public.h"

#include <xf86drm.h>

static const driOptionDescription amdgpu_2d_driconf[] = {
#include "pipe-loader/driinfo_gallium.h"
#include "radeonsi/driinfo_radeonsi.h"
};

enum { GPU2D_COPY, GPU2D_FILL, GPU2D_READ };

struct gpu2d_msg
{
    struct Message              msg;
    int                         op;
    struct Amdgpu_KMS           *kms;
    struct BitmapData           *bm;
    LONG                        sx, sy, dx, dy, w, h;
    ULONG                       pixel;
    UBYTE                       *dst;
    ULONG                       dstmod;
    BOOL                        result;
};

static struct
{
    struct SignalSemaphore      lock;
    struct Process              *worker;
    struct MsgPort              *port;
    struct Task                 *parent;
    BOOL                        tried;
    int                         fd;
    struct pipe_screen          *screen;
    struct pipe_context         *pipe;
    driOptionCache              option_info;
    driOptionCache              option_cache;
    struct pipe_resource        *scanout;
    ULONG                       scanout_handle;
    APTR                        scanout_map;
    struct pipe_resource        *staging;
} gpu2d;

void Amdgpu_2D_Init(void)
{
    memset(&gpu2d, 0, sizeof(gpu2d));
    gpu2d.fd = -1;
    InitSemaphore(&gpu2d.lock);
}

static BOOL amdgpu_2d_ready(void)
{
    struct pipe_screen_config config = { 0 };

    if (gpu2d.pipe)
        return TRUE;
    if (gpu2d.tried)
        return FALSE;
    gpu2d.tried = TRUE;

    gpu2d.fd = drmOpen(NULL, NULL);
    if (gpu2d.fd < 0)
        return FALSE;
    driParseOptionInfo(&gpu2d.option_info, amdgpu_2d_driconf, ARRAY_SIZE(amdgpu_2d_driconf));
    config.options_info = &gpu2d.option_info;
    config.options = &gpu2d.option_cache;
    gpu2d.screen = radeonsi_screen_create(gpu2d.fd, &config);
    if (gpu2d.screen)
        gpu2d.pipe = gpu2d.screen->context_create(gpu2d.screen, NULL, 0);
    if (!gpu2d.pipe)
    {
        bug("[Amdgpu:2D] no GPU context, 2D stays on the CPU\n");
        return FALSE;
    }
    return TRUE;
}

static struct pipe_resource *amdgpu_2d_scanout(struct Amdgpu_KMS *kms, struct BitmapData *bm)
{
    struct Amdgpu_FB *fb = &bm->fb;
    struct drm_gem_flink flink;
    struct winsys_handle wh;
    struct pipe_resource templ;

    if (gpu2d.scanout && gpu2d.scanout_handle == fb->handle && gpu2d.scanout_map == fb->map)
        return gpu2d.scanout;
    pipe_resource_reference(&gpu2d.scanout, NULL);

    memset(&flink, 0, sizeof(flink));
    flink.handle = fb->handle;
    if (drmIoctl(kms->fd, DRM_IOCTL_GEM_FLINK, &flink) != 0)
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

    gpu2d.scanout = gpu2d.screen->resource_from_handle(gpu2d.screen, &templ, &wh,
                                                       PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if (gpu2d.scanout)
    {
        gpu2d.scanout_handle = fb->handle;
        gpu2d.scanout_map = fb->map;
    }
    return gpu2d.scanout;
}

static struct pipe_resource *amdgpu_2d_texture(ULONG width, ULONG height, enum pipe_resource_usage usage)
{
    struct pipe_resource templ;

    memset(&templ, 0, sizeof(templ));
    templ.target = PIPE_TEXTURE_2D;
    templ.format = PIPE_FORMAT_B8G8R8X8_UNORM;
    templ.width0 = width;
    templ.height0 = height;
    templ.depth0 = 1;
    templ.array_size = 1;
    templ.usage = usage;
    templ.bind = usage == PIPE_USAGE_STAGING ? 0 : PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
    return gpu2d.screen->resource_create(gpu2d.screen, &templ);
}

static void amdgpu_2d_finish(void)
{
    struct pipe_fence_handle *fence = NULL;

    gpu2d.pipe->flush(gpu2d.pipe, &fence, 0);
    if (fence)
    {
        gpu2d.screen->fence_finish(gpu2d.screen, NULL, fence, OS_TIMEOUT_INFINITE);
        gpu2d.screen->fence_reference(gpu2d.screen, &fence, NULL);
    }
}

static void amdgpu_2d_box(struct pipe_box *box, LONG x, LONG y, LONG w, LONG h)
{
    memset(box, 0, sizeof(*box));
    box->x = x;
    box->y = y;
    box->width = w;
    box->height = h;
    box->depth = 1;
}

static BOOL gpu2d_copy(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG sx, LONG sy,
                       LONG dx, LONG dy, LONG w, LONG h)
{
    struct pipe_resource *scr, *tmp;
    struct pipe_box box;
    BOOL ok = FALSE;

    if (w <= 0 || h <= 0)
        return TRUE;

    if (amdgpu_2d_ready() && (scr = amdgpu_2d_scanout(kms, bm)))
    {
        BOOL overlap = sx < dx + w && dx < sx + w && sy < dy + h && dy < sy + h;

        amdgpu_2d_box(&box, sx, sy, w, h);
        if (!overlap)
        {
            gpu2d.pipe->resource_copy_region(gpu2d.pipe, scr, 0, dx, dy, 0, scr, 0, &box);
            ok = TRUE;
        }
        else if ((tmp = amdgpu_2d_texture(w, h, PIPE_USAGE_DEFAULT)))
        {
            gpu2d.pipe->resource_copy_region(gpu2d.pipe, tmp, 0, 0, 0, 0, scr, 0, &box);
            amdgpu_2d_box(&box, 0, 0, w, h);
            gpu2d.pipe->resource_copy_region(gpu2d.pipe, scr, 0, dx, dy, 0, tmp, 0, &box);
            pipe_resource_reference(&tmp, NULL);
            ok = TRUE;
        }
        if (ok)
            amdgpu_2d_finish();
    }
    return ok;
}

static BOOL gpu2d_fill(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h, ULONG pixel)
{
    struct pipe_resource *scr;
    struct pipe_box box;
    BOOL ok = FALSE;

    if (w <= 0 || h <= 0)
        return TRUE;

    if (amdgpu_2d_ready() && (scr = amdgpu_2d_scanout(kms, bm)))
    {
        amdgpu_2d_box(&box, x, y, w, h);
        gpu2d.pipe->clear_texture(gpu2d.pipe, scr, 0, &box, &pixel);
        amdgpu_2d_finish();
        ok = TRUE;
    }
    return ok;
}

static BOOL gpu2d_read(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h,
                    UBYTE *dst, ULONG dstmod)
{
    struct pipe_resource *scr;
    struct pipe_transfer *transfer = NULL;
    struct pipe_box box;
    UBYTE *src;
    BOOL ok = FALSE;
    LONG row;

    if (w <= 0 || h <= 0)
        return TRUE;

    if (amdgpu_2d_ready() && (scr = amdgpu_2d_scanout(kms, bm)))
    {
        if (gpu2d.staging && (gpu2d.staging->width0 < (ULONG)w || gpu2d.staging->height0 < (ULONG)h))
            pipe_resource_reference(&gpu2d.staging, NULL);
        if (!gpu2d.staging)
            gpu2d.staging = amdgpu_2d_texture(bm->fb.width, bm->fb.height, PIPE_USAGE_STAGING);
        if (gpu2d.staging)
        {
            amdgpu_2d_box(&box, x, y, w, h);
            gpu2d.pipe->resource_copy_region(gpu2d.pipe, gpu2d.staging, 0, 0, 0, 0, scr, 0, &box);
            src = pipe_texture_map(gpu2d.pipe, gpu2d.staging, 0, 0, PIPE_MAP_READ, 0, 0, w, h, &transfer);
            if (src)
            {
                for (row = 0; row < h; row++)
                    memcpy(dst + row * dstmod, src + row * transfer->stride, w * 4);
                pipe_texture_unmap(gpu2d.pipe, transfer);
                ok = TRUE;
            }
        }
    }
    return ok;
}

static void gpu2d_worker(void)
{
    struct gpu2d_msg *m;

    gpu2d.port = CreateMsgPort();
    if (gpu2d.port)
        amdgpu_2d_ready();
    Signal(gpu2d.parent, SIGF_SINGLE);
    if (!gpu2d.port)
        return;

    for (;;)
    {
        WaitPort(gpu2d.port);
        while ((m = (struct gpu2d_msg *)GetMsg(gpu2d.port)))
        {
            switch (m->op)
            {
                case GPU2D_COPY:
                    m->result = gpu2d_copy(m->kms, m->bm, m->sx, m->sy, m->dx, m->dy, m->w, m->h);
                    break;
                case GPU2D_FILL:
                    m->result = gpu2d_fill(m->kms, m->bm, m->dx, m->dy, m->w, m->h, m->pixel);
                    break;
                case GPU2D_READ:
                    m->result = gpu2d_read(m->kms, m->bm, m->sx, m->sy, m->w, m->h, m->dst, m->dstmod);
                    break;
                default:
                    m->result = FALSE;
            }
            ReplyMsg(&m->msg);
        }
    }
}

static BOOL gpu2d_call(struct gpu2d_msg *m)
{
    struct MsgPort port;
    BYTE sigbit;

    ObtainSemaphore(&gpu2d.lock);
    if (!gpu2d.worker && !gpu2d.tried)
    {
        gpu2d.parent = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        gpu2d.worker = CreateNewProcTags(NP_Name, (IPTR)"Amdgpu 2D",
                                         NP_Priority, 21,
                                         NP_Affinity, TASKAFFINITY_ANY,
                                         NP_Entry, (IPTR)gpu2d_worker,
                                         NP_StackSize, 256 * 1024,
                                         TAG_DONE);
        if (gpu2d.worker)
            Wait(SIGF_SINGLE);
    }
    if (!gpu2d.port || !gpu2d.pipe)
    {
        ReleaseSemaphore(&gpu2d.lock);
        return FALSE;
    }

    sigbit = AllocSignal(-1);
    if (sigbit < 0)
    {
        ReleaseSemaphore(&gpu2d.lock);
        return FALSE;
    }
    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags = PA_SIGNAL;
    port.mp_SigBit = sigbit;
    port.mp_SigTask = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);

    m->msg.mn_Node.ln_Type = NT_MESSAGE;
    m->msg.mn_ReplyPort = &port;
    m->msg.mn_Length = sizeof(*m);
    PutMsg(gpu2d.port, &m->msg);
    WaitPort(&port);
    GetMsg(&port);
    FreeSignal(sigbit);
    ReleaseSemaphore(&gpu2d.lock);
    return m->result;
}

BOOL Amdgpu_2D_CopyBox(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG sx, LONG sy,
                       LONG dx, LONG dy, LONG w, LONG h)
{
    struct gpu2d_msg m;

    if (w <= 0 || h <= 0)
        return TRUE;
    memset(&m, 0, sizeof(m));
    m.op = GPU2D_COPY;
    m.kms = kms; m.bm = bm;
    m.sx = sx; m.sy = sy; m.dx = dx; m.dy = dy; m.w = w; m.h = h;
    return gpu2d_call(&m);
}

BOOL Amdgpu_2D_Fill(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h, ULONG pixel)
{
    struct gpu2d_msg m;

    if (w <= 0 || h <= 0)
        return TRUE;
    memset(&m, 0, sizeof(m));
    m.op = GPU2D_FILL;
    m.kms = kms; m.bm = bm;
    m.dx = x; m.dy = y; m.w = w; m.h = h; m.pixel = pixel;
    return gpu2d_call(&m);
}

BOOL Amdgpu_2D_Read(struct Amdgpu_KMS *kms, struct BitmapData *bm, LONG x, LONG y, LONG w, LONG h,
                    UBYTE *dst, ULONG dstmod)
{
    struct gpu2d_msg m;

    if (w <= 0 || h <= 0)
        return TRUE;
    memset(&m, 0, sizeof(m));
    m.op = GPU2D_READ;
    m.kms = kms; m.bm = bm;
    m.sx = x; m.sy = y; m.w = w; m.h = h; m.dst = dst; m.dstmod = dstmod;
    return gpu2d_call(&m);
}
