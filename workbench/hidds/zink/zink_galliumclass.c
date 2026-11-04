/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/format/u_format.h"
#include "zink/zink_public.h"
#include "driver_trace/tr_public.h"
#include "frontend/winsys_handle.h"
#include "drm-uapi/drm_fourcc.h"

#include <proto/oop.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <cybergraphx/cybergraphics.h>

#include <string.h>

#include <hidd/gallium.h>
#include <hidd/gfx.h>
#include <hidd/vulkan_hidd.h>
#include <gallium/gallium.h>

#include <stdlib.h>

#include "zink_intern.h"

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_RAW   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

#define CyberGfxBase    (&BASE(cl->UserData)->sd)->CyberGfxBase
#define UtilityBase    (&BASE(cl->UserData)->sd)->UtilityBase

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->hiddGalliumAB)

/* Pixel layout WritePixelArray is given for a resource format: the pipe
 * format names bytes from the lowest address, as the RECTFMT_*32 do. */
/*
 * zink talks to the Vulkan driver directly, without a loader in between,
 * and some instance queries are the loader's to answer: a driver need not
 * implement layer enumeration (NVK does not), and before Vulkan 1.1 has no
 * vkEnumerateInstanceVersion. zink is handed this vkGetInstanceProcAddr,
 * which answers those as a loader without layers would and passes the rest
 * to the driver's. zink keeps one instance for all its screens, so one
 * driver entry point for the module is enough.
 */
static PFN_vkGetInstanceProcAddr zink_driver_gipa;

static VKAPI_ATTR VkResult VKAPI_CALL HiddZink_EnumerateInstanceLayerProperties(uint32_t *count,
                                                                                VkLayerProperties *props)
{
    (void)props;
    *count = 0;
    return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL HiddZink_EnumerateInstanceVersion(uint32_t *version)
{
    *version = VK_API_VERSION_1_0;
    return VK_SUCCESS;
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL HiddZink_GetInstanceProcAddr(VkInstance instance, const char *name)
{
    PFN_vkVoidFunction fn;

    if (!strcmp(name, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction)HiddZink_GetInstanceProcAddr;
    fn = zink_driver_gipa(instance, name);
    if (fn || instance)
        return fn;
    if (!strcmp(name, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction)HiddZink_EnumerateInstanceLayerProperties;
    if (!strcmp(name, "vkEnumerateInstanceVersion"))
        return (PFN_vkVoidFunction)HiddZink_EnumerateInstanceVersion;
    return NULL;
}

static ULONG HiddZink_RectFmt(enum pipe_format fmt)
{
#if (AROS_BIG_ENDIAN == 1)
    (void)fmt;
    return AROS_PIXFMT;
#else
    switch (fmt)
    {
    case PIPE_FORMAT_R8G8B8A8_UNORM:
    case PIPE_FORMAT_R8G8B8X8_UNORM:
    case PIPE_FORMAT_R8G8B8A8_SRGB:
        return RECTFMT_RGBA32;
    case PIPE_FORMAT_A8R8G8B8_UNORM:
    case PIPE_FORMAT_X8R8G8B8_UNORM:
    case PIPE_FORMAT_A8R8G8B8_SRGB:
        return RECTFMT_ARGB32;
    case PIPE_FORMAT_A8B8G8R8_UNORM:
    case PIPE_FORMAT_X8B8G8R8_UNORM:
    case PIPE_FORMAT_A8B8G8R8_SRGB:
        return RECTFMT_ABGR32;
    case PIPE_FORMAT_B5G6R5_UNORM:
        return RECTFMT_RGB16PC;
    default:
        return AROS_PIXFMT;
    }
#endif
}

/* The context DisplayResource reads resources back through. Rendering
 * contexts belong to the client; zink waits for their flushed work on the
 * resource when it is mapped here. */
static struct pipe_context *HiddZink_ReadbackContext(struct HiddGalliumZinkData *data)
{
    if (!data->zink_readctx && data->zink_screen)
        data->zink_readctx = data->zink_screen->context_create(data->zink_screen, NULL, 0);
    return data->zink_readctx;
}

static void HiddZink_DestroyScreen(struct HiddGalliumZinkData *data)
{
    if (data->zink_readctx)
    {
        data->zink_readctx->destroy(data->zink_readctx);
        data->zink_readctx = NULL;
    }
    if (data->client_screen)
    {
        /* the trace wrapper destroys the screen it wraps */
        data->client_screen->destroy(data->client_screen);
        data->client_screen = NULL;
        data->zink_screen = NULL;
    }
    else if (data->zink_screen)
    {
        data->zink_screen->destroy(data->zink_screen);
        data->zink_screen = NULL;
    }
}

OOP_Object *METHOD(HiddZink, Root, New)
{
    IPTR interfaceVers;
    OOP_Object *vkobj;
    PFN_vkGetInstanceProcAddr gipa = NULL;

    D(bug("[Zink] %s()\n", __PRETTY_FUNCTION__));

    interfaceVers = GetTagData(aHidd_Gallium_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != GALLIUM_INTERFACE_VERSION)
        return NULL;

    /* The Vulkan object this screen renders through. It is handed over
     * with the attribute: this object owns it from here on and disposes
     * it with itself (or right away, when creation fails), so the caller
     * must not dispose it. */
    vkobj = (OOP_Object *)GetTagData(aHidd_Gallium_VulkanObject, 0, msg->attrList);
    if (!vkobj)
    {
        bug("[Zink] %s: no Vulkan object\n", __PRETTY_FUNCTION__);
        return NULL;
    }

    {
        struct pHidd_Vulkan_GetInstanceProcAddr gipamsg =
        {
            .mID        = SD(cl)->vulkanMB + moHidd_Vulkan_GetInstanceProcAddr,
            .instance   = NULL,
            .pName      = "vkGetInstanceProcAddr",
        };
        gipa = (PFN_vkGetInstanceProcAddr)OOP_DoMethod(vkobj, (OOP_Msg)&gipamsg);
    }
    if (!gipa)
    {
        bug("[Zink] %s: Vulkan object %p has no vkGetInstanceProcAddr\n", __PRETTY_FUNCTION__, vkobj);
        OOP_DisposeObject(vkobj);
        return NULL;
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddGalliumZinkData *data = OOP_INST_DATA(cl, o);

        data->zink_obj = o;
        data->zink_vkobj = vkobj;
        data->zink_gipa = gipa;
        D(bug("[Zink] %s: vulkan object %p, gipa %p\n", __PRETTY_FUNCTION__, vkobj, gipa));
    }
    else
        OOP_DisposeObject(vkobj);

    return o;
}

VOID METHOD(HiddZink, Root, Dispose)
{
    struct HiddGalliumZinkData *data = OOP_INST_DATA(cl, o);

    D(bug("[Zink] %s()\n", __PRETTY_FUNCTION__));

    HiddZink_DestroyScreen(data);

    if (data->zink_vkobj)
    {
        OOP_DisposeObject(data->zink_vkobj);
        data->zink_vkobj = NULL;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddZink, Root, Get)
{
    ULONG idx;

    D(bug("[Zink] %s()\n", __PRETTY_FUNCTION__));

    if (IS_GALLIUM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            /* Overload the property */
            case aoHidd_Gallium_InterfaceVersion:
                *msg->storage = GALLIUM_INTERFACE_VERSION;
                return;

            /* DisplayResource reads frames through pipe_texture_map,
             * which detiles: colour buffers stay in the GPU's tiled
             * layout, which renders faster and needs no linear shadow. */
            case aoHidd_Gallium_LinearDisplay:
                *msg->storage = FALSE;
                return;
        }
    }

    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

APTR METHOD(HiddZink, Hidd_Gallium, CreatePipeScreen)
{
    struct HiddGalliumZinkData *data = OOP_INST_DATA(cl, o);

    D(bug("[Zink] %s()\n", __PRETTY_FUNCTION__));

    if (data->zink_screen)
    {
        bug("[Zink] %s: screen %p already created\n", __PRETTY_FUNCTION__, data->zink_screen);
        return NULL;
    }

    /* No window system: the screen is created without a sw_winsys and
     * without driconf. Resources are plain Vulkan images (mesa3dgl creates
     * its colour buffers without PIPE_BIND_DISPLAY_TARGET, so zink never
     * reaches for a kopper swapchain); DisplayResource reads them back. */
    zink_driver_gipa = data->zink_gipa;
    data->zink_screen = zink_create_screen_with_gipa(NULL, NULL, HiddZink_GetInstanceProcAddr);
    if (!data->zink_screen)
        bug("[Zink] %s: no screen; Mesa's messages go to the file named by MESA_LOG_FILE\n", __PRETTY_FUNCTION__);

    /* With GALLIUM_TRACE set (a file name), Mesa's trace driver records
     * every call the client makes on the screen and its contexts there;
     * without it the same screen comes back. DisplayResource keeps reading
     * through zink's own screen. */
    data->client_screen = data->zink_screen ? trace_screen_create(data->zink_screen) : NULL;

    D(bug("[Zink] %s: screen @ 0x%p (client 0x%p)\n", __PRETTY_FUNCTION__, data->zink_screen,
          data->client_screen));

    return data->client_screen;
}

VOID METHOD(HiddZink, Hidd_Gallium, DestroyPipeScreen)
{
    struct HiddGalliumZinkData *data = OOP_INST_DATA(cl, o);

    D(bug("[Zink] %s(%p)\n", __PRETTY_FUNCTION__, msg->screen));

    if (msg->screen && msg->screen != data->client_screen)
    {
        bug("[Zink] %s: screen %p is not this object's (%p)\n", __PRETTY_FUNCTION__,
            msg->screen, data->client_screen);
        return;
    }

    HiddZink_DestroyScreen(data);
}

VOID METHOD(HiddZink, Hidd_Gallium, DisplayResource)
{
    struct HiddGalliumZinkData *data = OOP_INST_DATA(cl, o);
    struct pipe_resource *res = (struct pipe_resource *)msg->resource;
    struct pipe_context *ctx;
    struct pipe_transfer *xfer = NULL;
    APTR map;

    D(bug("[Zink] %s()\n", __PRETTY_FUNCTION__));

    if (!res || !msg->bitmap || !msg->width || !msg->height)
        return;

    ctx = HiddZink_ReadbackContext(data);
    if (!ctx)
    {
        bug("[Zink] %s: no readback context\n", __PRETTY_FUNCTION__);
        return;
    }

    map = pipe_texture_map(ctx, res, 0, 0, PIPE_MAP_READ,
                           msg->srcx, msg->srcy, msg->width, msg->height, &xfer);
    if (map)
    {
        struct RastPort *rp = CreateRastPort();

        D(bug("[Zink] %s: resource %p fmt %d map %p stride %u -> bitmap %p %u,%u %ux%u\n",
              __PRETTY_FUNCTION__, res, res->format, map, xfer->stride, msg->bitmap,
              msg->dstx, msg->dsty, msg->width, msg->height));

        if (rp)
        {
            rp->BitMap = msg->bitmap;

            WritePixelArray(
                map,
                0,
                0,
                xfer->stride,
                rp,
                msg->dstx,
                msg->dsty,
                msg->width,
                msg->height,
                HiddZink_RectFmt(res->format));

            FreeRastPort(rp);
        }

        pipe_texture_unmap(ctx, xfer);
    }
    else
        bug("[Zink] %s: mapping resource %p failed\n", __PRETTY_FUNCTION__, res);
}

/*
 * Buffer `buffer` of a scan-out bitmap of the display as a pipe_resource of
 * this pipe's screen (gallium.library's scan-out framebuffers): GL renders
 * straight into what the display shows, and a swap becomes a flip instead
 * of DisplayResource's readback and copy.
 *
 * The buffer crosses into Vulkan the way a dma-buf would: its GEM handle in
 * the display's DRM client travels in the fd's place, and NVK's DRM layer
 * opens that buffer in its own client (drmPrimeFDToHandle). Scan-out
 * buffers are linear, so the import names the LINEAR modifier and the
 * bitmap's pitch; NVK then renders through a tiled shadow of the image when
 * a depth buffer is bound, and copies it into the buffer on the GPU.
 */
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#define HiddBitMapAttrBase  (SD(cl)->hiddBitMapAB)
#define HiddPixFmtAttrBase  (SD(cl)->hiddPixFmtAB)

APTR METHOD(HiddZink, Hidd_Gallium, ResourceFromBitMap)
{
    struct pipe_screen *pscreen = (struct pipe_screen *)msg->screen;
    OOP_Object *bm = msg->bitmap;
    OOP_Object *pixfmt = NULL;
    IPTR width = 0, height = 0, pitch = 0, stdpf = 0, handle;
    struct winsys_handle whandle;
    struct pipe_resource templ;
    struct pipe_resource *res;
    enum pipe_format format;

    if (!pscreen || !bm || !pscreen->resource_from_handle ||
        !SD(cl)->hiddBitMapAB || !SD(cl)->hiddPixFmtAB)
        return NULL;

    {
        struct pHidd_BitMap_GetBufferHandle gbh =
        {
            .mID    = SD(cl)->bitmapMB + moHidd_BitMap_GetBufferHandle,
            .buffer = msg->buffer,
        };
        handle = OOP_DoMethod(bm, (OOP_Msg)&gbh);
    }
    OOP_GetAttr(bm, aHidd_BitMap_Width, &width);
    OOP_GetAttr(bm, aHidd_BitMap_Height, &height);
    /* The buffer's own pitch: the scan-out allocation's row alignment is
     * the driver's (wider than a width-based BytesPerRow), and the display
     * reads rows at exactly this stride. Lock only to learn it; nothing is
     * written, so the unlock names an empty rectangle. */
    {
        UBYTE *addr = NULL;
        ULONG lpitch = 0;
        struct pHidd_BitMap_LockBuffer lb =
        {
            .mID           = SD(cl)->bitmapMB + moHidd_BitMap_LockBuffer,
            .buffer        = msg->buffer,
            .addressReturn = &addr,
            .pitchReturn   = &lpitch,
            .planeOffsets  = NULL,
        };

        if (OOP_DoMethod(bm, (OOP_Msg)&lb))
        {
            struct Rectangle none = { 0, 0, -1, -1 };
            struct pHidd_BitMap_UnlockBuffer ub =
            {
                .mID    = SD(cl)->bitmapMB + moHidd_BitMap_UnlockBuffer,
                .buffer = msg->buffer,
                .dirty  = &none,
            };

            OOP_DoMethod(bm, (OOP_Msg)&ub);
            pitch = lpitch;
        }
    }
    OOP_GetAttr(bm, aHidd_BitMap_PixFmt, (IPTR *)&pixfmt);
    if (pixfmt)
        OOP_GetAttr(pixfmt, aHidd_PixFmt_StdPixFmt, &stdpf);

    if (!handle || !width || !height || !pitch)
    {
        bug("[Zink] ResourceFromBitMap: bitmap %p buffer %lu: handle %lu %lux%lu pitch %lu, refused\n",
            bm, (unsigned long)msg->buffer, (unsigned long)handle, (unsigned long)width,
            (unsigned long)height, (unsigned long)pitch);
        return NULL;
    }

    switch (stdpf)
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
        bug("[Zink] ResourceFromBitMap: std pixfmt %lu has no pipe format\n", (unsigned long)stdpf);
        return NULL;
    }

    memset(&whandle, 0, sizeof(whandle));
    whandle.type     = WINSYS_HANDLE_TYPE_FD;
    whandle.handle   = (unsigned)handle;
    whandle.stride   = pitch;
    whandle.offset   = 0;
    whandle.modifier = DRM_FORMAT_MOD_LINEAR;
    whandle.format   = format;

    memset(&templ, 0, sizeof(templ));
    templ.target     = PIPE_TEXTURE_2D;
    templ.format     = format;
    templ.width0     = width;
    templ.height0    = height;
    templ.depth0     = 1;
    templ.array_size = 1;
    templ.bind       = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW
                     | PIPE_BIND_SCANOUT | PIPE_BIND_LINEAR | PIPE_BIND_SHARED;
    templ.usage      = PIPE_USAGE_DEFAULT;

    res = pscreen->resource_from_handle(pscreen, &templ, &whandle,
                                        PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE);

    if (!res)
        bug("[Zink] ResourceFromBitMap: bitmap %p buffer %lu handle %lu %lux%lu pitch %lu fmt %d -> %p\n",
            bm, (unsigned long)msg->buffer, (unsigned long)handle, (unsigned long)width,
            (unsigned long)height, (unsigned long)pitch, (int)format, res);

    return res;
}
