/*
    Copyright 2015-2026, The AROS Development Team. All rights reserved.

    Desc: radeonsi.amdd's gallium class: Mesa's radeonsi driver over its
          stock amdgpu winsys and a libdrm of its own (amdd_drm.c), a DRM
          client per screen. Made by init_class (amdd.h) as a subclass of the
          base class amdgpu.hidd passes, with the driver's services in
          struct amdd_host.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <cybergraphx/cybergraphics.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <gallium/gallium.h>
#include <hidd/gallium.h>
#include <amdd/amdd.h>

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/os_time.h"
#include "util/driconf.h"
#include "frontend/winsys_handle.h"
#include "radeonsi/si_public.h"

#include <xf86drm.h>
#include <libdrm/drm_entry.h>
#include <stdlib.h>
#include <string.h>

#include "amdd_gallium.h"

#include LC_LIBDEFS_FILE

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT     RECTFMT_RAW
#else
#define AROS_PIXFMT     RECTFMT_BGRA32
#endif

#define AMDGPU_SWAP_MAILBOX     0xFFFFFFFFUL

OOP_AttrBase HiddGalliumAttrBase;

struct amdd_host amdd_host;
static BOOL amdd_host_ready;
static OOP_Class *gl_class;
static LONG gl_objects;             /* objects alive */
static struct SignalSemaphore gl_lock;

static const driOptionDescription amdd_driconf[] = {
#include "pipe-loader/driinfo_gallium.h"
#include "radeonsi/driinfo_radeonsi.h"
};

/* One pipe screen per GL context, with a presenter showing the newest
   frame of its mailbox at the next vblank */
struct amdd_gallium_data
{
    struct amdd_screen          scr;
    struct pipe_context         *pipe;          /* for reading resources back */
    struct amdd_scanout         scanout;        /* the screen's buffer, as a gallium resource */
    ULONG                       swap_interval;  /* vblanks per present, AMDGPU_SWAP_MAILBOX, 0 = no sync */

    struct SignalSemaphore      mbox_lock;
    struct pipe_resource        *mbox;
    struct pipe_fence_handle    *mbox_fence;
    OOP_Object                  *mbox_bm;
    struct pipe_box             mbox_src[32];
    LONG                        mbox_dstx[32], mbox_dsty[32];
    ULONG                       mbox_count;
    volatile BOOL               mbox_dirty;
    struct pipe_context         *present_pipe;
    struct Task                 *presenter;
    struct Task                 *presenter_parent;
    ULONG                       presenter_ack;  /* parent's signal for start/stop handshakes */
    ULONG                       presenter_sigmask;
    volatile BOOL               presenter_stop;
    volatile BOOL               presenter_done;
};

static void amdd_presenter_stop(struct amdd_gallium_data *data);

/* ---- the host, the screen and the scan-out buffers ---- */

BOOL amdd_host_set(const struct amdd_host *host)
{
    if (amdd_host_ready)
        return TRUE;
    if (!host || host->version != AMDD_INTERFACE_VERSION || !host->drm)
        return FALSE;
    if (host->size < offsetof(struct amdd_host, wait_vblank) + sizeof(host->wait_vblank))
        return FALSE;

    memset(&amdd_host, 0, sizeof(amdd_host));
    CopyMem((APTR)host, &amdd_host, (host->size < sizeof(amdd_host)) ? host->size : sizeof(amdd_host));
    amdd_host.size = sizeof(amdd_host);
    amdd_drm = (const struct amdgpu_drm_entry *)amdd_host.drm;
    amdd_host_ready = TRUE;
    return TRUE;
}

/* A screen with a DRM client of its own, as Mesa's DRM winsys makes it */
BOOL amdd_screen_open(struct amdd_screen *s)
{
    struct pipe_screen_config config = { 0 };

    s->fd = drmOpen(NULL, NULL);
    if (s->fd < 0)
    {
        amdd_log("[RadeonSI] no DRM client (%d)\n", s->fd);
        return FALSE;
    }
    if (!s->option_info.info)
        driParseOptionInfo(&s->option_info, amdd_driconf, ARRAY_SIZE(amdd_driconf));
    config.options_info = &s->option_info;
    config.options = &s->option_cache;
    s->screen = radeonsi_screen_create(s->fd, &config);
    if (!s->screen)
    {
        amdd_log("[RadeonSI] no screen\n");
        drmClose(s->fd);
        s->fd = -1;
        return FALSE;
    }
    return TRUE;
}

void amdd_screen_close(struct amdd_screen *s)
{
    if (s->screen)
    {
        s->screen->destroy(s->screen);
        s->screen = NULL;
    }
    if (s->fd >= 0)
    {
        drmClose(s->fd);
        s->fd = -1;
    }
}

/* The driver's scan-out buffer of `bm` as a resource of `screen`: the
   driver names it (GEM flink) and the screen's client opens it by that
   name - the one buffer, nothing is copied. */
struct pipe_resource *amdd_scanout_get(struct pipe_screen *screen, OOP_Object *bm, struct amdd_scanout *so)
{
    struct amdd_buffer buf;
    struct winsys_handle wh;
    struct pipe_resource templ;

    if (!amdd_host.bitmap_buffer || !amdd_host.bitmap_buffer(bm, 0, &buf))
        return NULL;
    if (so->res && so->id == buf.id)
        return so->res;
    pipe_resource_reference(&so->res, NULL);
    if (buf.stdfmt != vHidd_StdPixFmt_BGR032)
        return NULL;

    memset(&wh, 0, sizeof(wh));
    wh.type = WINSYS_HANDLE_TYPE_SHARED;
    wh.handle = buf.name;
    wh.stride = buf.pitch;
    wh.format = PIPE_FORMAT_B8G8R8X8_UNORM;

    memset(&templ, 0, sizeof(templ));
    templ.target = PIPE_TEXTURE_2D;
    templ.format = PIPE_FORMAT_B8G8R8X8_UNORM;
    templ.width0 = buf.width;
    templ.height0 = buf.height;
    templ.depth0 = 1;
    templ.array_size = 1;
    templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SCANOUT | PIPE_BIND_SHARED;

    so->res = screen->resource_from_handle(screen, &templ, &wh, PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
    if (so->res)
    {
        uint64_t stride = 0, offset = 0, modifier = 0;

        so->id = buf.id;
        if (screen->resource_get_param)
        {
            screen->resource_get_param(screen, NULL, so->res, 0, 0, 0, PIPE_RESOURCE_PARAM_STRIDE, 0, &stride);
            screen->resource_get_param(screen, NULL, so->res, 0, 0, 0, PIPE_RESOURCE_PARAM_OFFSET, 0, &offset);
            screen->resource_get_param(screen, NULL, so->res, 0, 0, 0, PIPE_RESOURCE_PARAM_MODIFIER, 0, &modifier);
        }
        amdd_log("[RadeonSI] scan-out %lu: name %lu, %lux%lu, pitch %lu; as imported: stride %llu, offset %llu, modifier 0x%llx\n",
                 (unsigned long)buf.id, (unsigned long)buf.name, (unsigned long)buf.width, (unsigned long)buf.height,
                 (unsigned long)buf.pitch, (unsigned long long)stride, (unsigned long long)offset,
                 (unsigned long long)modifier);
    }
    else
        amdd_log("[RadeonSI] scan-out %lu (name %lu, pitch %lu) cannot be imported\n",
                 (unsigned long)buf.id, (unsigned long)buf.name, (unsigned long)buf.pitch);
    return so->res;
}

void amdd_screen_finish(struct pipe_screen *screen, struct pipe_context *pipe)
{
    struct pipe_fence_handle *fence = NULL;

    pipe->flush(pipe, &fence, 0);
    if (fence)
    {
        screen->fence_finish(screen, NULL, fence, OS_TIMEOUT_INFINITE);
        screen->fence_reference(screen, &fence, NULL);
    }
}

/* ---- the class ---- */

static OOP_Object *AMDDGallium__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    IPTR interfaceVers;

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct amdd_gallium_data *data = OOP_INST_DATA(cl, o);
        char value[8];

        memset(data, 0, sizeof(*data));
        data->scr.fd = -1;
        data->swap_interval = AMDGPU_SWAP_MAILBOX;
        InitSemaphore(&data->mbox_lock);
        if (GetVar("AMDGPU_SWAP_INTERVAL", value, sizeof(value), 0) > 0)
            data->swap_interval = strtoul(value, NULL, 10);
        __sync_fetch_and_add(&gl_objects, 1);
    }
    return o;
}

static VOID AMDDGallium__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct amdd_gallium_data *data = OOP_INST_DATA(cl, o);

    if (data->scr.option_cache.info)
        driDestroyOptionCache(&data->scr.option_cache);
    if (data->scr.option_info.info)
        driDestroyOptionInfo(&data->scr.option_info);
    __sync_fetch_and_sub(&gl_objects, 1);
    OOP_DoSuperMethod(cl, o, msg);
}

static VOID AMDDGallium__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gallium_InterfaceVersion:
            *msg->storage = GALLIUM_INTERFACE_VERSION;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

static APTR AMDDGallium__Hidd_Gallium__CreatePipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_CreatePipeScreen *msg)
{
    struct amdd_gallium_data *data = OOP_INST_DATA(cl, o);

    if (data->scr.screen)
        return data->scr.screen;
    if (!amdd_screen_open(&data->scr))
        return NULL;
    data->pipe = data->scr.screen->context_create(data->scr.screen, data, 0);
    return data->scr.screen;
}

static VOID AMDDGallium__Hidd_Gallium__DestroyPipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DestroyPipeScreen *msg)
{
    struct amdd_gallium_data *data = OOP_INST_DATA(cl, o);
    struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

    if (!screen || screen != data->scr.screen)
        return;

    amdd_presenter_stop(data);
    pipe_resource_reference(&data->scanout.res, NULL);
    if (data->pipe)
    {
        data->pipe->destroy(data->pipe);
        data->pipe = NULL;
    }
    amdd_screen_close(&data->scr);
}

static void amdd_blit(struct pipe_context *pipe, struct pipe_resource *src, const struct pipe_box *box,
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

/* Shows the newest frame of the mailbox once per vblank. */
static void amdd_presenter(void)
{
    struct amdd_gallium_data *data = FindTask(NULL)->tc_UserData;
    BYTE sigbit = AllocSignal(-1);

    data->presenter_sigmask = sigbit >= 0 ? 1UL << sigbit : 0;
    Signal(data->presenter_parent, data->presenter_ack);
    if (sigbit < 0)
        return;

    while (!data->presenter_stop)
    {
        OOP_Object *bm;
        struct pipe_resource *dst;
        ULONG i;

        if (!data->mbox_dirty)
        {
            Wait(data->presenter_sigmask);
            continue;
        }
        bm = data->mbox_bm;
        if (bm && amdd_host.wait_vblank)
            amdd_host.wait_vblank(bm);

        ObtainSemaphore(&data->mbox_lock);
        bm = data->mbox_bm;
        if (data->mbox_dirty && bm && data->mbox)
        {
            if (data->mbox_fence)
                data->scr.screen->fence_finish(data->scr.screen, NULL, data->mbox_fence, OS_TIMEOUT_INFINITE);
            amdd_host.bitmap_lock(bm, TRUE);
            dst = amdd_scanout_get(data->scr.screen, bm, &data->scanout);
            if (dst)
            {
                for (i = 0; i < data->mbox_count; i++)
                    amdd_blit(data->present_pipe, data->mbox, &data->mbox_src[i], dst,
                              data->mbox_dstx[i], data->mbox_dsty[i]);
                amdd_screen_finish(data->scr.screen, data->present_pipe);
            }
            amdd_host.bitmap_lock(bm, FALSE);
        }
        data->mbox_dirty = FALSE;
        ReleaseSemaphore(&data->mbox_lock);
    }
    FreeSignal(sigbit);
    data->presenter_done = TRUE;
    Signal(data->presenter_parent, data->presenter_ack);
}

static BOOL amdd_presenter_start(struct amdd_gallium_data *data)
{
    BYTE ack;

    if (data->presenter)
        return TRUE;
    data->present_pipe = data->scr.screen->context_create(data->scr.screen, NULL, 0);
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
    data->presenter = (struct Task *)CreateNewProcTags(NP_Name, (IPTR)"RadeonSI Present",
                                                       NP_Priority, 1,
                                                       NP_Entry, (IPTR)amdd_presenter,
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

static void amdd_presenter_stop(struct amdd_gallium_data *data)
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
        data->scr.screen->fence_reference(data->scr.screen, &data->mbox_fence, NULL);
}

/* Queue a frame region for the presenter; a region overlapping one already
   queued starts a new frame. */
static BOOL amdd_display_mailbox(struct amdd_gallium_data *data, OOP_Object *bm,
                                 struct pipe_resource *res, struct pHidd_Gallium_DisplayResource *msg)
{
    struct pipe_box box;
    ULONG i;

    if (!amdd_presenter_start(data))
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
        data->mbox = data->scr.screen->resource_create(data->scr.screen, &templ);
        data->mbox_count = 0;
    }
    if (!data->mbox)
    {
        ReleaseSemaphore(&data->mbox_lock);
        return FALSE;
    }
    if (data->mbox_bm != bm)
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
        data->scr.screen->fence_reference(data->scr.screen, &data->mbox_fence, NULL);
    data->pipe->flush(data->pipe, &data->mbox_fence, 0);
    data->mbox_bm = bm;
    data->mbox_dirty = TRUE;
    ReleaseSemaphore(&data->mbox_lock);

    Signal(data->presenter, data->presenter_sigmask);
    return TRUE;
}

/* The rendered resource into the driver's scan-out buffer, on the GPU */
static BOOL amdd_display_gpu(struct amdd_gallium_data *data, struct pHidd_Gallium_DisplayResource *msg)
{
    OOP_Object *bm = HIDD_BM_OBJ(msg->bitmap);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_resource *dst;
    struct amdd_buffer buf;
    struct pipe_box box;

    if (!bm || !amdd_host.bitmap_buffer || !amdd_host.bitmap_buffer(bm, 0, &buf))
        return FALSE;

    if (data->swap_interval == AMDGPU_SWAP_MAILBOX)
        return amdd_display_mailbox(data, bm, res, msg);

    if (amdd_host.wait_vblank)
    {
        ULONG n;

        for (n = 0; n < data->swap_interval; n++)
            if (!amdd_host.wait_vblank(bm))
                break;
    }

    memset(&box, 0, sizeof(box));
    box.x = msg->srcx;
    box.y = msg->srcy;
    box.width = msg->width;
    box.height = msg->height;
    box.depth = 1;

    amdd_host.bitmap_lock(bm, TRUE);
    dst = amdd_scanout_get(data->scr.screen, bm, &data->scanout);
    if (dst)
    {
        amdd_blit(data->pipe, res, &box, dst, msg->dstx, msg->dsty);
        amdd_screen_finish(data->scr.screen, data->pipe);
    }
    amdd_host.bitmap_lock(bm, FALSE);

    return dst != NULL;
}

static VOID AMDDGallium__Hidd_Gallium__DisplayResource(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DisplayResource *msg)
{
    struct amdd_gallium_data *data = OOP_INST_DATA(cl, o);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_context *pipe = data->pipe;
    struct pipe_transfer *transfer = NULL;
    uint8_t *mapped;
    struct RastPort *rp;

    if (!pipe || !res || !msg->bitmap)
        return;

    if (amdd_display_gpu(data, msg))
        return;

    /* any other bitmap: through the CPU */
    mapped = pipe_texture_map(pipe, res, 0, 0, PIPE_MAP_READ,
                              msg->srcx, msg->srcy, msg->width, msg->height, &transfer);
    if (!mapped)
    {
        amdd_log("[RadeonSI] texture_map failed\n");
        return;
    }

    rp = CreateRastPort();
    if (rp)
    {
        rp->BitMap = msg->bitmap;
        WritePixelArray(mapped, 0, 0, transfer->stride, rp,
                        msg->dstx, msg->dsty, msg->width, msg->height, AROS_PIXFMT);
        FreeRastPort(rp);
    }
    pipe_texture_unmap(pipe, transfer);
}

/* ---- library interface (include/amdd/amdd.h) ---- */

static OOP_Class *amdd_init_class(OOP_Class *base, const struct amdd_host *host)
{
    OOP_Class *cl;

    if (!base)
        return NULL;

    ObtainSemaphore(&gl_lock);
    if (!gl_class && amdd_host_set(host))
    {
        struct OOP_MethodDescr root_descr[] =
        {
            {(OOP_MethodFunc)AMDDGallium__Root__New, moRoot_New},
            {(OOP_MethodFunc)AMDDGallium__Root__Dispose, moRoot_Dispose},
            {(OOP_MethodFunc)AMDDGallium__Root__Get, moRoot_Get},
            {NULL, 0}
        };
        struct OOP_MethodDescr gallium_descr[] =
        {
            {(OOP_MethodFunc)AMDDGallium__Hidd_Gallium__CreatePipeScreen, moHidd_Gallium_CreatePipeScreen},
            {(OOP_MethodFunc)AMDDGallium__Hidd_Gallium__DestroyPipeScreen, moHidd_Gallium_DestroyPipeScreen},
            {(OOP_MethodFunc)AMDDGallium__Hidd_Gallium__DisplayResource, moHidd_Gallium_DisplayResource},
            {NULL, 0}
        };
        struct OOP_InterfaceDescr ifdescr[] =
        {
            {root_descr, IID_Root, 3},
            {gallium_descr, IID_Hidd_Gallium, 3},
            {NULL, NULL, 0}
        };
        OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        struct TagItem tags[] =
        {
            {aMeta_SuperPtr, (IPTR)base},
            {aMeta_InterfaceDescr, (IPTR)ifdescr},
            {aMeta_InstSize, sizeof(struct amdd_gallium_data)},
            {TAG_DONE, 0}
        };

        if (!HiddGalliumAttrBase)
            HiddGalliumAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gallium);
        if (HiddGalliumAttrBase)
            gl_class = (OOP_Class *)OOP_NewObject(NULL, CLID_HiddMeta, tags);
    }
    cl = gl_class;
    ReleaseSemaphore(&gl_lock);

    return cl;
}

static OOP_Object *amdd_new_object(OOP_Object *display, struct TagItem *attrList)
{
    if (!gl_class)
        return NULL;
    return OOP_NewObject(gl_class, NULL, attrList);
}

static BOOL amdd_free_class(void)
{
    BOOL freed = FALSE;

    ObtainSemaphore(&gl_lock);
    if (gl_objects == 0)
    {
        if (gl_class)
            OOP_DisposeObject((OOP_Object *)gl_class);
        gl_class = NULL;
        if (HiddGalliumAttrBase)
            OOP_ReleaseAttrBase(IID_Hidd_Gallium);
        HiddGalliumAttrBase = 0;
        freed = TRUE;
    }
    ReleaseSemaphore(&gl_lock);

    return freed;
}

static const struct amdd_interface amdd_interface =
{
    sizeof(struct amdd_interface),
    AMDD_INTERFACE_VERSION,
    amdd_init_class,
    amdd_new_object,
    amdd_free_class,
    NULL
};

AROS_LH0(const struct amdd_interface *, AMDDInterface,
    LIBBASETYPEPTR, LIBBASE, 5, RadeonSI)
{
    AROS_LIBFUNC_INIT

    return &amdd_interface;

    AROS_LIBFUNC_EXIT
}


static int RadeonSI_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&gl_lock);
    return TRUE;
}

ADD2INITLIB(RadeonSI_Init, 0)
ADD2LIBS((STRPTR)"cybergraphics.library", 0, struct Library *, CyberGfxBase);
