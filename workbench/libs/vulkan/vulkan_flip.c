/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: swapchains presented by scan-out.

    The swapchain's images are the buffers of a scan-out bitmap of the
    window's display (as gallium.library's scan-out framebuffers are): the
    driver imports each buffer as the memory of a linear image, the bitmap
    is attached to the display as a client surface over the window's
    rectangle, and presenting an image is a buffer swap. The display shows
    the bitmap through a hardware plane when the compositor grants one and
    blits it through the window's visible region otherwise; nothing is
    copied by the library either way.

    The surface follows the window with the surfacetrack helper, as
    gallium.library and cgxvideo.library do.

    Only the buffer the display has released can be rendered next, so one
    image at a time is the application's.

    Lock order: vkl_wsi_lock -> flip->lock -> tracker lock -> layer lock.
    The surface hook runs in the damaging task and only Attempts.
*/

#include <aros/debug.h>
#include <aros/asmcall.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <intuition/intuition.h>
#include <hidd/gfx.h>
#include <aros/surfacetrack.h>

#include <string.h>

#include "vulkan_wsi.h"

#define VKL_FLIP_MAXBUFFERS 3

/* DRM_FORMAT_MOD_LINEAR */
#define VKL_MOD_LINEAR  0ULL

const char * const vkl_flip_exts[VKL_FLIP_NUM_EXTS] =
{
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
};

struct vkl_flip
{
    struct SignalSemaphore          lock;
    struct vkl_swapchain           *sc;

    OOP_Object                     *display;    /* the window's screen's display */
    OOP_Object                     *bm;         /* the scan-out bitmap */
    struct Window                  *window;     /* NULL once detached */

    struct HIDD_CompositeRequest    req;
    struct Hook                     hook;       /* compositor -> us */
    struct SurfaceTrack             track;
    BOOL                            tracking;   /* st_Init done */
    struct Rectangle                inner;      /* layer relative rectangle */

    ULONG                           buffers;
    ULONG                           back;       /* the buffer to render next */
    volatile BOOL                   onplane;
    BOOL                            attached;
    BOOL                            nowait;     /* swaps do not wait for the display */
};

/* the attribute base the <hidd/gfx.h> attribute names use */
OOP_AttrBase HiddBitMapAttrBase;

static OOP_Class *vkl_basebm;

/* SetEnv VK_DEBUG 1: say on the debug output why a swapchain is not, or how
   it is, presented by scan-out */
static BOOL vkl_flip_debug;
#define FLIPLOG(...) do { if (vkl_flip_debug) bug("[Vulkan] flip: " __VA_ARGS__); } while (0)
static struct SurfaceTrackNotify vkl_notify;
static BOOL vkl_notify_active;

/* ---- the display side ---- */

/* the display object of the screen a window is on */
static OOP_Object *vkl_window_display(struct Window *window, OOP_Object **screenbmp)
{
    struct BitMap *bitmap;
    OOP_Object *screenbm;
    IPTR display = 0;

    if (!window->WScreen || !HiddBitMapAttrBase)
        return NULL;
    bitmap = window->WScreen->RastPort.BitMap;
    if (!bitmap || !IS_HIDD_BM(bitmap) || !(screenbm = HIDD_BM_OBJ(bitmap)))
        return NULL;
    OOP_GetAttr(screenbm, aHidd_BitMap_Display, &display);
    *screenbmp = screenbm;
    return (OOP_Object *)display;
}

/* copy the tracker's result into the request and tell the compositor; flip->lock held */
static void vkl_flip_push(struct vkl_flip *flip)
{
    flip->req.Dest = flip->track.Dest;
    flip->req.Shape = flip->track.Shape;
    if (flip->attached)
        HIDD_Display_UpdateSurface(flip->display, &flip->req);
}

/* a recompute the hook had to leave; flip->lock held, may wait for the layer */
static void vkl_flip_refresh(struct vkl_flip *flip)
{
    if (!flip->attached || !flip->window || !st_IsStale(&flip->track))
        return;
    if (st_Compute(&flip->track, flip->window, &flip->inner, NULL, NULL, TRUE) && st_Changed(&flip->track))
        vkl_flip_push(flip);
}

/* compositor -> client: runs in the damaging task and must not block */
AROS_UFH3(static IPTR, vkl_flip_hook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, display, A2),
    AROS_UFHA(struct HIDD_SurfaceHookMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct vkl_flip *flip = hook->h_Data;

    if (!msg)
        return 0;

    switch (msg->Type)
    {
    case vHidd_SurfaceHook_PlaneGained:
        flip->onplane = TRUE;
        return 0;
    case vHidd_SurfaceHook_PlaneLost:
        flip->onplane = FALSE;
        return 0;
    case vHidd_SurfaceHook_Damage:
        break;
    default:
        return 0;
    }

    if (!AttemptSemaphore(&flip->lock))
    {
        st_SetStale(&flip->track);
        return 0;
    }
    if (flip->attached && flip->window && st_HookDamageRect(&flip->track, msg, &flip->inner))
    {
        st_Changed(&flip->track);
        vkl_flip_push(flip);
    }
    ReleaseSemaphore(&flip->lock);
    return 0;

    AROS_USERFUNC_EXIT
}

/* off the display; flip->lock held */
static void vkl_flip_detach_locked(struct vkl_flip *flip)
{
    if (!flip->attached)
        return;
    HIDD_Display_DetachSurface(flip->display, &flip->req);
    flip->attached = FALSE;
    flip->onplane = FALSE;
    flip->window = NULL;
    st_Reset(&flip->track);
    flip->req.Shape = NULL;
}

void vkl_flip_detach(struct vkl_swapchain *sc)
{
    struct vkl_flip *flip = sc ? sc->flip : NULL;

    if (!flip)
        return;
    ObtainSemaphore(&flip->lock);
    vkl_flip_detach_locked(flip);
    ReleaseSemaphore(&flip->lock);
}

/*
 * A window is about to close (in the closing task): its surfaces are lost,
 * and what they have on the display comes off it.
 */
static void vkl_close_notify(struct Window *window, APTR userData)
{
    struct MinNode *n;

    ObtainSemaphore(&vkl_wsi_lock);
    for (n = vkl_wsi_surfaces.mlh_Head; n->mln_Succ; n = n->mln_Succ)
    {
        struct vkl_surface *s = (struct vkl_surface *)((UBYTE *)n - offsetof(struct vkl_surface, allnode));

        if (s->window != window || s->lost)
            continue;

        /* what the surface was, for queries made after this */
        if (!s->subrect)
        {
            LONG x, y, w, h;

            vkl_surface_rect(s, &x, &y, &w, &h);
            s->width = w;
            s->height = h;
        }
        s->lost = TRUE;
        if (s->swapchain)
            vkl_flip_detach(s->swapchain);
    }
    ReleaseSemaphore(&vkl_wsi_lock);
}

void vkl_flip_init(void)
{
    HiddBitMapAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);
    vkl_basebm = OOP_FindClass(CLID_Hidd_BitMap);
    vkl_notify_active = st_StartCloseNotify(&vkl_notify, vkl_close_notify, NULL);
}

void vkl_flip_exit(void)
{
    if (vkl_notify_active)
        st_EndCloseNotify(&vkl_notify);
    vkl_notify_active = FALSE;
    if (HiddBitMapAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);
    HiddBitMapAttrBase = 0;
}

/* ---- the driver side ---- */

/* a buffer's row pitch: lock it only to learn that */
static ULONG vkl_buffer_pitch(OOP_Object *bm, ULONG buffer)
{
    UBYTE *addr = NULL;
    ULONG pitch = 0;

    if (HIDD_BM_LockBuffer(bm, buffer, &addr, &pitch, NULL))
    {
        struct Rectangle none = { 0, 0, -1, -1 };

        HIDD_BM_UnlockBuffer(bm, buffer, &none);
        return pitch;
    }
    return 0;
}

/*
 * One buffer of the bitmap as an image: a linear image with the buffer's
 * pitch, its memory imported from the buffer's handle (which stands in the
 * place of a dma-buf file descriptor).
 */
static VkResult vkl_flip_import(struct vkl_swapchain *sc, const VkImageCreateInfo *base, ULONG buffer,
    const VkAllocationCallbacks *alloc)
{
    struct vkl_device *dev = sc->dev;
    struct vkl_flip *flip = sc->flip;
    struct vkl_image *img = &sc->images[buffer];
    IPTR handle = HIDD_BM_GetBufferHandle(flip->bm, buffer);
    ULONG pitch = vkl_buffer_pitch(flip->bm, buffer);
    VkSubresourceLayout layout = { .offset = 0, .rowPitch = pitch };
    VkImageDrmFormatModifierExplicitCreateInfoEXT modinfo =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
        .pNext = base->pNext,
        .drmFormatModifier = VKL_MOD_LINEAR,
        .drmFormatModifierPlaneCount = 1,
        .pPlaneLayouts = &layout,
    };
    VkExternalMemoryImageCreateInfo extinfo =
    {
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .pNext = &modinfo,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
    };
    VkImageCreateInfo ici = *base;
    VkMemoryDedicatedAllocateInfo dedicated = { .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
    VkImportMemoryFdInfoKHR import =
    {
        .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
        .pNext = &dedicated,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
        .fd = (int)handle,
    };
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &import };
    VkMemoryFdPropertiesKHR fdprops = { .sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR };
    VkMemoryRequirements mr;
    uint32_t bits, i;
    VkResult res;

    if (!handle || !pitch)
    {
        FLIPLOG("buffer %lu has handle %lu, pitch %lu\n", (unsigned long)buffer,
                (unsigned long)handle, (unsigned long)pitch);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    ici.pNext = &extinfo;
    ici.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    res = dev->drv.CreateImage(dev->handle, &ici, alloc, &img->image);
    if (res != VK_SUCCESS)
        return res;

    dev->drv.GetImageMemoryRequirements(dev->handle, img->image, &mr);
    bits = mr.memoryTypeBits;
    if (dev->drv.GetMemoryFdPropertiesKHR &&
        dev->drv.GetMemoryFdPropertiesKHR(dev->handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
                                          (int)handle, &fdprops) == VK_SUCCESS &&
        (bits & fdprops.memoryTypeBits))
        bits &= fdprops.memoryTypeBits;
    for (i = 0; i < 32 && !(bits & (1u << i)); i++)
        ;
    if (i == 32)
        return VK_ERROR_INITIALIZATION_FAILED;

    dedicated.image = img->image;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = i;
    res = dev->drv.AllocateMemory(dev->handle, &mai, alloc, &img->memory);
    if (res != VK_SUCCESS)
        return res;
    return dev->drv.BindImageMemory(dev->handle, img->image, img->memory, 0);
}

void vkl_flip_destroy(struct vkl_swapchain *sc, const VkAllocationCallbacks *alloc)
{
    struct vkl_flip *flip = sc->flip;
    struct vkl_device *dev = sc->dev;
    uint32_t i;

    if (!flip)
        return;

    ObtainSemaphore(&flip->lock);
    vkl_flip_detach_locked(flip);
    ReleaseSemaphore(&flip->lock);

    for (i = 0; i < VKL_FLIP_MAXBUFFERS; i++)
    {
        if (sc->images[i].image)
            dev->drv.DestroyImage(dev->handle, sc->images[i].image, alloc);
        if (sc->images[i].memory)
            dev->drv.FreeMemory(dev->handle, sc->images[i].memory, alloc);
        sc->images[i].image = VK_NULL_HANDLE;
        sc->images[i].memory = VK_NULL_HANDLE;
    }
    sc->imageCount = 0;

    if (flip->bm)
        OOP_DisposeObject(flip->bm);
    if (flip->tracking)
        st_Cleanup(&flip->track);
    sc->flip = NULL;
    vkl_free(flip);
}

VkResult vkl_flip_create(struct vkl_swapchain *sc, const VkSwapchainCreateInfoKHR *info,
    const VkImageCreateInfo *ici, const VkAllocationCallbacks *alloc)
{
    struct vkl_device *dev = sc->dev;
    struct vkl_surface *s = sc->surface;
    struct Window *window = s->window;
    struct HIDD_ScanoutCaps caps;
    OOP_Object *display, *screenbm = NULL;
    struct vkl_flip *flip;
    HIDDT_StdPixFmt stdpf;
    struct Rectangle dest;
    struct Region *shape = NULL;
    ULONG buffers, i;
    IPTR val;
    char mode[16];
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    vkl_flip_debug = GetVar("VK_DEBUG", mode, sizeof(mode), 0) > 0;

    if (!dev->flipExts || !vkl_basebm || s->lost)
    {
        FLIPLOG("the driver does not import display buffers\n");
        return res;
    }

    /* VK_PRESENT=readback: never by scan-out */
    if (GetVar("VK_PRESENT", mode, sizeof(mode), 0) > 0 && !stricmp(mode, "readback"))
        return res;

    /* the device must be the one the window's display belongs to */
    display = vkl_window_display(window, &screenbm);
    if (!display || display != dev->icd->icd->display)
    {
        FLIPLOG("the window's display %p is not the device's %p\n", display, dev->icd->icd->display);
        return res;
    }

    switch (info->imageFormat)
    {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
        stdpf = vHidd_StdPixFmt_BGRA32;
        break;
    default:
        FLIPLOG("format %ld cannot be scanned out\n", (long)info->imageFormat);
        return res;
    }

    /* the application may hold one image at a time, so it gets no more
       images than it asks for at least; the display has 2 or 3 */
    buffers = info->minImageCount;
    if (buffers < 2)
        buffers = 2;
    if (buffers > VKL_FLIP_MAXBUFFERS)
    {
        FLIPLOG("%lu images asked for\n", (unsigned long)buffers);
        return res;
    }

    memset(&caps, 0, sizeof(caps));
    if (!(HIDD_Display_QueryScanout(display, stdpf, 0, sc->width, sc->height, &caps) & SCF_PLANE))
    {
        FLIPLOG("the display has no plane for %lux%lu\n", (unsigned long)sc->width, (unsigned long)sc->height);
        return res;
    }
    if (caps.MaxBuffers && buffers > caps.MaxBuffers)
    {
        FLIPLOG("the display allows %lu buffers\n", (unsigned long)caps.MaxBuffers);
        return res;
    }

    /* ... except that presenting without waiting for the display takes a
       third buffer */
    if (info->presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR && buffers < 3 &&
        (!caps.MaxBuffers || caps.MaxBuffers >= 3))
        buffers = 3;

    flip = vkl_alloc(sizeof(*flip));
    if (!flip)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    InitSemaphore(&flip->lock);
    flip->sc = sc;
    flip->display = display;
    sc->flip = flip;

    if (!st_Init(&flip->track, NULL))
        goto fail;
    flip->tracking = TRUE;

    /* the surface's rectangle in the window's layer */
    st_WindowInnerRect(window, 0, 0, 0, 0, &flip->inner);
    if (s->subrect)
    {
        flip->inner.MinX += s->left;
        flip->inner.MinY += s->top;
    }
    flip->inner.MaxX = flip->inner.MinX + sc->width - 1;
    flip->inner.MaxY = flip->inner.MinY + sc->height - 1;

    {
        struct TagItem bmtags[] =
        {
            { aHidd_BitMap_Scanout,   TRUE           },
            { aHidd_BitMap_Width,     sc->width      },
            { aHidd_BitMap_Height,    sc->height     },
            { aHidd_BitMap_Buffers,   buffers        },
            { aHidd_BitMap_StdPixFmt, (IPTR)stdpf    },
            { aHidd_BitMap_Friend,    (IPTR)screenbm },
            { TAG_DONE,               0              }
        };

        flip->bm = HIDD_Display_CreateObject(display, vkl_basebm, bmtags);
    }
    if (!flip->bm)
    {
        FLIPLOG("no scan-out bitmap\n");
        goto fail;
    }

    val = 0;
    OOP_GetAttr(flip->bm, aHidd_BitMap_Buffers, &val);
    flip->buffers = val;
    if (flip->buffers != buffers)
    {
        FLIPLOG("the bitmap has %lu buffers, not %lu\n", (unsigned long)flip->buffers, (unsigned long)buffers);
        goto fail;
    }

    /* not waiting for the display needs a third buffer: with two, the one
       rendered next is the one being displaced */
    flip->nowait = (flip->buffers >= 3) && (info->presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR);

    for (i = 0; i < flip->buffers; i++)
    {
        res = vkl_flip_import(sc, ici, i, alloc);
        if (res != VK_SUCCESS)
        {
            FLIPLOG("buffer %lu not imported (%ld)\n", (unsigned long)i, (long)res);
            goto fail;
        }
    }
    sc->imageCount = flip->buffers;
    res = VK_ERROR_INITIALIZATION_FAILED;

    /* onto the display */
    if (!st_Compute(&flip->track, window, &flip->inner, &dest, &shape, TRUE))
    {
        FLIPLOG("no surface geometry\n");
        goto fail;
    }
    st_Changed(&flip->track);

    flip->req.Next = NULL;
    flip->req.BitMap = flip->bm;
    flip->req.Src.MinX = 0;
    flip->req.Src.MinY = 0;
    flip->req.Src.MaxX = sc->width - 1;
    flip->req.Src.MaxY = sc->height - 1;
    flip->req.Dest = dest;
    flip->req.Shape = shape;
    flip->req.Opacity = 255;
    flip->req.AlphaMode = vHidd_CompAlpha_None;
    flip->req.Flags = CRF_PLANE_OK | CRF_OPAQUE;
    flip->req.ZOrder = 0;
    flip->req.UserData = NULL;

    flip->hook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(vkl_flip_hook);
    flip->hook.h_SubEntry = NULL;
    flip->hook.h_Data = flip;

    if (!HIDD_Display_AttachSurface(display, screenbm, &flip->req, &flip->hook))
    {
        FLIPLOG("the display refused the surface\n");
        st_Reset(&flip->track);
        flip->req.Shape = NULL;
        goto fail;
    }
    flip->window = window;
    flip->attached = TRUE;

    val = 0;
    OOP_GetAttr(flip->bm, aHidd_BitMap_BackBuffer, &val);
    flip->back = (val < flip->buffers) ? val : 0;

    FLIPLOG("%lux%lu, %lu buffers, back %lu, %s\n", (unsigned long)sc->width,
            (unsigned long)sc->height, (unsigned long)flip->buffers, (unsigned long)flip->back,
            flip->nowait ? "no wait" : "waits for the display");
    return VK_SUCCESS;

fail:
    vkl_flip_destroy(sc, alloc);
    return res;
}

BOOL vkl_flip_next(struct vkl_swapchain *sc, uint32_t *index)
{
    struct vkl_flip *flip = sc->flip;

    if (sc->images[flip->back].acquired)
        return FALSE;
    *index = flip->back;
    return TRUE;
}

/*
 * Present: wait until the image is complete - the display shows the buffer
 * as it is - then swap it onto the display. The buffer the display gives
 * back is the one to render next.
 */
VkResult vkl_flip_present(struct vkl_device *dev, VkQueue queue, struct vkl_swapchain *sc,
    uint32_t index, uint32_t waitCount, const VkSemaphore *waits)
{
    struct vkl_flip *flip = sc->flip;
    VkPipelineStageFlags stages[16];
    VkResult res;
    IPTR back = 0;
    uint32_t i;
    BOOL current;

    if (waitCount > 16)
        waitCount = 16;

    if (waitCount)
    {
        /* an empty batch behind the application's semaphores */
        VkSubmitInfo si =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = waitCount,
            .pWaitSemaphores = waits,
            .pWaitDstStageMask = stages,
        };

        for (i = 0; i < waitCount; i++)
            stages[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        res = vkl_queue_submit(dev, queue, 1, &si, sc->fence);
        if (res == VK_SUCCESS)
        {
            res = dev->drv.WaitForFences(dev->handle, 1, &sc->fence, VK_TRUE, UINT64_MAX);
            dev->drv.ResetFences(dev->handle, 1, &sc->fence);
        }
    }
    else
        res = dev->disp.QueueWaitIdle(queue);

    sc->images[index].acquired = FALSE;
    if (res != VK_SUCCESS)
        return res;

    if (sc->retired || !sc->surface)
        return VK_ERROR_OUT_OF_DATE_KHR;
    if (sc->surface->lost)
        return VK_ERROR_SURFACE_LOST_KHR;

    ObtainSemaphore(&flip->lock);
    if (!flip->attached)
    {
        ReleaseSemaphore(&flip->lock);
        return VK_ERROR_SURFACE_LOST_KHR;
    }

    /* a recompute the hook had to leave */
    vkl_flip_refresh(flip);

    HIDD_BM_SwapBuffers(flip->bm, index, flip->nowait ? fHidd_Swap_NoWait : 0);

    /* never a buffer that is still scanned out: the one the driver released */
    OOP_GetAttr(flip->bm, aHidd_BitMap_BackBuffer, &back);
    flip->back = (back < flip->buffers) ? back : 0;
    ReleaseSemaphore(&flip->lock);

    sc->surface->presentPath = VKPP_Flip;

    {
        LONG x, y, w, h;

        vkl_surface_rect(sc->surface, &x, &y, &w, &h);
        current = ((uint32_t)w == sc->width && (uint32_t)h == sc->height);
    }
    return current ? VK_SUCCESS : VK_SUBOPTIMAL_KHR;
}

IPTR vkl_flip_attr(struct vkl_swapchain *sc, ULONG attr)
{
    struct vkl_flip *flip = sc->flip;

    if (!flip)
        return 0;
    switch (attr)
    {
    case VKSA_OnPlane:
        return flip->onplane ? TRUE : FALSE;
    case VKSA_ScanoutBitMap:
        return (IPTR)flip->bm;
    case VKSA_Buffers:
        return flip->buffers;
    }
    return 0;
}
