/*
    Copyright 2010-2026, The AROS Development Team. All rights reserved.

    nouveau.nvdd: the nvgfx driver's gallium part - Mesa's nv30, nv50 and
    nvc0 drivers - as a library the driver opens when its display is first
    asked for a gallium object (include/nvdd/nvdd.h).

    The class is a subclass of the gallium.hidd base class the driver
    passes in; this library never opens gallium.hidd. Every pipe screen
    has a DRM client of its own, opened through the driver's DRM entry
    table (nvdd_drm.c), and owns it as on Linux: destroying the screen
    closes it. Buffers cross to the driver's display by GEM name, through
    the driver's services in struct nvdd_host.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <gallium/gallium.h>
#include <hidd/gallium.h>
#include <nvdd/nvdd.h>

#include <xf86drm.h>
#include "nouveau.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "nouveau_screen.h"
#include "nouveau_winsys.h"
#include "nv50/nv50_resource.h"
#include "nv30/nv30_resource.h"
#include "frontend/winsys_handle.h"

#include "libdrm/drm_entry.h"
#include "nvdd_gallium.h"

#include <string.h>

#include LC_LIBDEFS_FILE

OOP_AttrBase HiddGalliumAttrBase;

/* the driver's services and the class */
static struct nvdd_host gl_host;
static OOP_Class *gl_class;
static LONG gl_objects;             /* objects and pipe screens alive */
static struct SignalSemaphore gl_lock;

#define gl_log(...) do { if (gl_host.log) gl_host.log(__VA_ARGS__); } while (0)

typedef struct nouveau_screen *(*screen_create_func)(struct nouveau_device *);

/* The gallium screen constructor for a chipset, NULL when it has none */
static screen_create_func nvdd_screen_create_func(ULONG chipset)
{
    switch (chipset & ~0xf)
    {
    case 0x30: case 0x40: case 0x60:
        return nv30_screen_create;
    case 0x50: case 0x80: case 0x90: case 0xa0:
        return nv50_screen_create;
    case 0xc0: case 0xd0: case 0xe0: case 0xf0: case 0x100:
    case 0x110: case 0x120: case 0x130: case 0x140: case 0x160:
    case 0x170: case 0x190:
        return nvc0_screen_create;
    }
    return NULL;
}

/* The buffer object and level-0 pitch of a resource of one of the screens */
static struct nouveau_bo *nvdd_resource_bo(struct pipe_resource *resource, ULONG *pitch)
{
    switch (gl_host.chipset & ~0xf)
    {
    case 0x30: case 0x40: case 0x60:
        *pitch = nv30_miptree(resource)->level[0].pitch;
        return nv30_miptree(resource)->base.bo;
    default:
        *pitch = nv50_miptree(resource)->level[0].pitch;
        return nv50_miptree(resource)->base.bo;
    }
}

/* The nv30 family; nv50 and nvc0 are told apart from it only */
static BOOL nvdd_is_nv30(void)
{
    switch (gl_host.chipset & ~0xf)
    {
    case 0x30: case 0x40: case 0x60:
        return TRUE;
    }
    return FALSE;
}

/* The nv50 family (Tesla) */
static BOOL nvdd_is_nv50(void)
{
    switch (gl_host.chipset & ~0xf)
    {
    case 0x50: case 0x80: case 0x90: case 0xa0:
        return TRUE;
    }
    return FALSE;
}

/* ---- the class ---- */

static OOP_Object *NVDDGallium__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    IPTR interfaceVers;

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;
    if (!nvdd_screen_create_func(gl_host.chipset))
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
        __sync_fetch_and_add(&gl_objects, 1);
    return o;
}

static VOID NVDDGallium__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    __sync_fetch_and_sub(&gl_objects, 1);
    OOP_DoSuperMethod(cl, o, msg);
}

static VOID NVDDGallium__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gallium_InterfaceVersion:
            *msg->storage = GALLIUM_INTERFACE_VERSION;
            return;

        /* nv50 and nvc0 colour buffers stay tiled: the 2D engine copy in
           DisplayResource reads the tiled layout (the imported buffer
           carries its memtype and tile mode), and neither family renders
           a linear colour buffer with a depth buffer - nv50 rejects the
           draw (RT_LINEAR_WITH_ZETA), nvc0 drops the depth buffer. The
           nv30 2D copy cannot read a swizzled buffer, so nv30 keeps
           linear ones. */
        case aoHidd_Gallium_LinearDisplay:
            *msg->storage = nvdd_is_nv30();
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* A screen with a DRM client of its own, as Mesa's DRM winsys makes it */
static APTR NVDDGallium__Hidd_Gallium__CreatePipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_CreatePipeScreen *msg)
{
    screen_create_func init = nvdd_screen_create_func(gl_host.chipset);
    struct nouveau_drm *drm = NULL;
    struct nouveau_device *dev = NULL;
    struct nouveau_screen *screen;
    int fd;

    if (!init)
        return NULL;

    fd = drmOpen("nouveau", "");
    if (fd < 0)
    {
        gl_log("[NouveauGallium] no DRM client (%d)\n", fd);
        return NULL;
    }
    if (nouveau_drm_new(fd, &drm))
        goto err_drm;
    if (nouveau_device_new(&drm->client, &dev))
        goto err_dev;

    screen = init(dev);
    if (!screen)
        goto err_screen;
    if (!screen->base.context_create)
    {
        /* the screen owns the device, the DRM client and the fd now */
        screen->base.destroy(&screen->base);
        return NULL;
    }
    /* what the drm winsys would have done for us */
    screen->initialized = true;
    __sync_fetch_and_add(&gl_objects, 1);
    return screen;

err_screen:
    nouveau_device_del(&dev);
err_dev:
    nouveau_drm_del(&drm);
err_drm:
    drmClose(fd);
    gl_log("[NouveauGallium] no screen for chipset nv%02lx\n", (unsigned long)gl_host.chipset);
    return NULL;
}

static VOID NVDDGallium__Hidd_Gallium__DestroyPipeScreen(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DestroyPipeScreen *msg)
{
    struct pipe_screen *screen = (struct pipe_screen *)msg->screen;

    if (!screen)
        return;
    screen->destroy(screen);
    __sync_fetch_and_sub(&gl_objects, 1);
}

static VOID NVDDGallium__Hidd_Gallium__DisplayResource(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_DisplayResource *msg)
{
    struct pipe_resource *resource = (struct pipe_resource *)msg->resource;
    struct nouveau_bo *bo;
    ULONG pitch = 0, depth;
    uint32_t name = 0;

    if (!resource || !gl_host.display_copy)
        return;

    switch (resource->format)
    {
    case PIPE_FORMAT_B8G8R8A8_UNORM:
    case PIPE_FORMAT_A8R8G8B8_UNORM:
        /* A 32-bit render buffer is shown as a 24-bit surface, so the
           copy's (src->depth == dst->depth) check passes */
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
        return;
    }

    bo = nvdd_resource_bo(resource, &pitch);
    if (!bo || !pitch || nouveau_bo_name_get(bo, &name) || !name)
        return;

    gl_host.display_copy(msg->bitmap, name, pitch, depth,
        resource->width0, resource->height0,
        msg->srcx, msg->srcy, msg->dstx, msg->dsty, msg->width, msg->height);
}

/*
 * Alias buffer `buffer` of the driver's scan-out bitmap as a pipe_resource
 * of `screen` (3D scan-out): the driver names it (GEM flink) and the
 * screen's client opens it by that name. Linear (pitch) buffers only - the
 * plane and the compositor read them by pitch.
 */
static APTR NVDDGallium__Hidd_Gallium__ResourceFromBitMap(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gallium_ResourceFromBitMap *msg)
{
    struct pipe_screen *pscreen = (struct pipe_screen *)msg->screen;
    struct nvdd_buffer buf;
    struct winsys_handle whandle;
    struct pipe_resource templ;
    enum pipe_format format;

    if (!pscreen || !msg->bitmap || !pscreen->resource_from_handle || !gl_host.bitmap_buffer)
        return NULL;
    /* An nv50 linear colour buffer cannot be rendered with a depth buffer
       bound (the draw fails with RT_LINEAR_WITH_ZETA): no scan-out buffers,
       GL keeps tiled colour buffers and the DisplayResource copy. */
    if (nvdd_is_nv50())
        return NULL;
    if (!gl_host.bitmap_buffer(msg->bitmap, msg->buffer, &buf))
        return NULL;
    if (buf.memtype != 0)
        return NULL;

    switch (buf.stdfmt)
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
        return NULL;
    }

    memset(&whandle, 0, sizeof(whandle));
    whandle.type   = WINSYS_HANDLE_TYPE_SHARED;
    whandle.handle = buf.name;
    whandle.stride = buf.pitch;
    whandle.offset = 0;

    memset(&templ, 0, sizeof(templ));
    templ.target     = pscreen->caps.npot_textures ? PIPE_TEXTURE_2D : PIPE_TEXTURE_RECT;
    templ.format     = format;
    templ.width0     = buf.width;
    templ.height0    = buf.height;
    templ.depth0     = 1;
    templ.array_size = 1;
    templ.bind       = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW
                     | PIPE_BIND_SCANOUT | PIPE_BIND_LINEAR | PIPE_BIND_SHARED;
    templ.usage      = PIPE_USAGE_DEFAULT;

    return pscreen->resource_from_handle(pscreen, &templ, &whandle,
                                         PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);
}

/* ---- library interface (include/nvdd/nvdd.h) ---- */

AROS_LH2(OOP_Class *, NVDDInitClass,
    AROS_LHA(OOP_Class *, base, A0),
    AROS_LHA(const struct nvdd_host *, host, A1),
    LIBBASETYPEPTR, LIBBASE, 5, NouveauGallium)
{
    AROS_LIBFUNC_INIT

    OOP_Class *cl;

    if (!base || !host || host->version != NVDD_INTERFACE_VERSION || !host->drm)
        return NULL;
    /* the gallium services came later than the common part */
    if (host->size < offsetof(struct nvdd_host, display_copy) + sizeof(host->display_copy))
        return NULL;

    ObtainSemaphore(&gl_lock);
    if (!gl_class)
    {
        struct OOP_MethodDescr root_descr[] =
        {
            {(OOP_MethodFunc)NVDDGallium__Root__New, moRoot_New},
            {(OOP_MethodFunc)NVDDGallium__Root__Dispose, moRoot_Dispose},
            {(OOP_MethodFunc)NVDDGallium__Root__Get, moRoot_Get},
            {NULL, 0}
        };
        struct OOP_MethodDescr gallium_descr[] =
        {
            {(OOP_MethodFunc)NVDDGallium__Hidd_Gallium__CreatePipeScreen, moHidd_Gallium_CreatePipeScreen},
            {(OOP_MethodFunc)NVDDGallium__Hidd_Gallium__DestroyPipeScreen, moHidd_Gallium_DestroyPipeScreen},
            {(OOP_MethodFunc)NVDDGallium__Hidd_Gallium__DisplayResource, moHidd_Gallium_DisplayResource},
            {(OOP_MethodFunc)NVDDGallium__Hidd_Gallium__ResourceFromBitMap, moHidd_Gallium_ResourceFromBitMap},
            {NULL, 0}
        };
        struct OOP_InterfaceDescr ifdescr[] =
        {
            {root_descr, IID_Root, 3},
            {gallium_descr, IID_Hidd_Gallium, 4},
            {NULL, NULL, 0}
        };
        OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        struct TagItem tags[] =
        {
            {aMeta_SuperPtr, (IPTR)base},
            {aMeta_InterfaceDescr, (IPTR)ifdescr},
            {aMeta_InstSize, sizeof(ULONG)},
            {TAG_DONE, 0}
        };

        memset(&gl_host, 0, sizeof(gl_host));
        CopyMem((APTR)host, &gl_host, (host->size < sizeof(gl_host)) ? host->size : sizeof(gl_host));
        gl_host.size = sizeof(gl_host);
        nvdd_drm = (const struct nouveau_drm_entry *)gl_host.drm;
        nvdd_log = gl_host.log;

        if (!HiddGalliumAttrBase)
            HiddGalliumAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gallium);
        if (HiddGalliumAttrBase)
        {
            gl_class = (OOP_Class *)OOP_NewObject(NULL, CLID_HiddMeta, tags);
            if (gl_class)
                gl_class->UserData = LIBBASE;
        }
    }
    cl = gl_class;
    ReleaseSemaphore(&gl_lock);

    return cl;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(OOP_Object *, NVDDNewObject,
    AROS_LHA(OOP_Object *, display, A0),
    AROS_LHA(struct TagItem *, attrList, A1),
    LIBBASETYPEPTR, LIBBASE, 6, NouveauGallium)
{
    AROS_LIBFUNC_INIT

    if (!gl_class)
        return NULL;
    return OOP_NewObject(gl_class, NULL, attrList);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BOOL, NVDDFreeClass,
    LIBBASETYPEPTR, LIBBASE, 7, NouveauGallium)
{
    AROS_LIBFUNC_INIT

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

    AROS_LIBFUNC_EXIT
}

static int NouveauGallium_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&gl_lock);
    return TRUE;
}

ADD2INITLIB(NouveauGallium_Init, 0)
