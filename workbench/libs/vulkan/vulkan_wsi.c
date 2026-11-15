/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: window system integration - VK_KHR_surface,
    VK_AROS_surface and VK_KHR_swapchain, implemented here for every driver
    with nothing but the driver's core Vulkan entry points.

    A surface is the inner area (or a rectangle of it) of an Intuition
    window. A swapchain's images are ordinary images the library creates on
    the device; presenting one copies it into a host visible buffer and
    writes that into the window's RastPort (WritePixelArray), which takes
    care of the window's layer: clipping, depth arrangement, damage.

    Presentation is synchronous: an image is free again when
    vkQueuePresentKHR returns, so acquiring one never has to wait.
    Acquisition signals the application's semaphore and fence with an empty
    submission on a queue of the device (see vulkan_device.c for the lock).
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>

#include <string.h>

#include "vulkan_intern.h"

#include "vulkan_wsi.h"

/* ---- surfaces ---- */

struct SignalSemaphore vkl_wsi_lock;
struct MinList vkl_wsi_surfaces;

/* the surface's rectangle in its window's RastPort, as it is now */
void vkl_surface_rect(struct vkl_surface *s, LONG *x, LONG *y, LONG *w, LONG *h)
{
    struct Window *win = s->window;
    LONG ix, iy, iw, ih;

    if (s->lost)
    {
        /* the window is gone: what the surface last was */
        *x = *y = 0;
        *w = s->width > 0 ? s->width : 1;
        *h = s->height > 0 ? s->height : 1;
        return;
    }

    if (win->Flags & WFLG_GIMMEZEROZERO)
    {
        ix = 0;
        iy = 0;
        iw = win->GZZWidth;
        ih = win->GZZHeight;
    }
    else
    {
        ix = win->BorderLeft;
        iy = win->BorderTop;
        iw = win->Width - win->BorderLeft - win->BorderRight;
        ih = win->Height - win->BorderTop - win->BorderBottom;
    }

    if (s->subrect)
    {
        *x = ix + s->left;
        *y = iy + s->top;
        *w = s->width;
        *h = s->height;
    }
    else
    {
        *x = ix;
        *y = iy;
        *w = iw;
        *h = ih;
    }
    if (*w < 1)
        *w = 1;
    if (*h < 1)
        *h = 1;
}

static VkResult vkl_surface_create(struct vkl_instance *inst, struct Window *window, BOOL subrect,
    LONG left, LONG top, LONG width, LONG height, VkSurfaceKHR *pSurface)
{
    struct vkl_surface *s;

    if (!inst || inst->magic != VKL_INSTANCE_MAGIC || !window || !window->RPort)
        return VK_ERROR_INITIALIZATION_FAILED;
    if (subrect && (width <= 0 || height <= 0))
        return VK_ERROR_INITIALIZATION_FAILED;

    s = vkl_alloc(sizeof(*s));
    if (!s)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    s->magic = VKL_SURFACE_MAGIC;
    s->instance = inst;
    s->window = window;
    s->subrect = subrect;
    s->left = left;
    s->top = top;
    s->width = width;
    s->height = height;

    ObtainSemaphore(&inst->lock);
    ADDTAIL((struct List *)&inst->surfaces, (struct Node *)&s->node);
    ReleaseSemaphore(&inst->lock);
    ObtainSemaphore(&vkl_wsi_lock);
    ADDTAIL((struct List *)&vkl_wsi_surfaces, (struct Node *)&s->allnode);
    ReleaseSemaphore(&vkl_wsi_lock);

    *pSurface = (VkSurfaceKHR)(IPTR)s;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkACreateSurface(VkInstance instance, struct TagItem *tagList,
    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
    struct Window *window = (struct Window *)GetTagData(VKA_Window, 0, tagList);
    BOOL subrect = FindTagItem(VKA_Width, tagList) && FindTagItem(VKA_Height, tagList);

    return vkl_surface_create(VKL_INSTANCE(instance), window, subrect,
        GetTagData(VKA_Left, 0, tagList), GetTagData(VKA_Top, 0, tagList),
        GetTagData(VKA_Width, 0, tagList), GetTagData(VKA_Height, 0, tagList), pSurface);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAROSSurfaceAROS(VkInstance instance,
    const VkAROSSurfaceCreateInfoAROS *pCreateInfo, const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface)
{
    return vkl_surface_create(VKL_INSTANCE(instance), pCreateInfo->window, FALSE, 0, 0, 0, 0, pSurface);
}

VKAPI_ATTR IPTR VKAPI_CALL vkAGetSurfaceAttr(VkInstance instance, VkSurfaceKHR surface, ULONG attr)
{
    struct vkl_surface *s = VKL_SURFACE(surface);
    LONG x, y, w, h;

    if (!s || s->magic != VKL_SURFACE_MAGIC)
        return 0;

    switch (attr)
    {
    case VKSA_Window:
        return (IPTR)s->window;
    case VKSA_Width:
        vkl_surface_rect(s, &x, &y, &w, &h);
        return w;
    case VKSA_Height:
        vkl_surface_rect(s, &x, &y, &w, &h);
        return h;
    case VKSA_PresentPath:
        return s->presentPath;
    case VKSA_OnPlane:
    case VKSA_ScanoutBitMap:
    case VKSA_Buffers:
        return s->swapchain ? vkl_flip_attr(s->swapchain, attr) : 0;
    }
    return 0;
}

static void vkl_surface_free(struct vkl_surface *s)
{
    ObtainSemaphore(&vkl_wsi_lock);
    REMOVE((struct Node *)&s->allnode);
    ReleaseSemaphore(&vkl_wsi_lock);

    /* a swapchain that outlives its surface can no longer present */
    if (s->swapchain)
    {
        vkl_flip_detach(s->swapchain);
        s->swapchain->retired = TRUE;
        s->swapchain->surface = NULL;
    }
    s->magic = 0;
    vkl_free(s);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface,
    const VkAllocationCallbacks *pAllocator)
{
    struct vkl_instance *inst = VKL_INSTANCE(instance);
    struct vkl_surface *s = VKL_SURFACE(surface);

    if (!s || s->magic != VKL_SURFACE_MAGIC)
        return;

    ObtainSemaphore(&inst->lock);
    REMOVE((struct Node *)&s->node);
    ReleaseSemaphore(&inst->lock);
    vkl_surface_free(s);
}

void vkl_wsi_instance_cleanup(struct vkl_instance *inst)
{
    struct vkl_surface *s;

    while ((s = (struct vkl_surface *)REMHEAD((struct List *)&inst->surfaces)))
        vkl_surface_free(s);
}

/* ---- surface queries ---- */

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported)
{
    struct vkl_icd_instance *ii = VKL_PHYSDEV_ICD(physicalDevice);
    VkQueueFamilyProperties props[32];
    uint32_t count = 32;

    /* presenting is a copy out of the image: any queue that can transfer */
    *pSupported = VK_FALSE;
    ii->disp.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, props);
    if (queueFamilyIndex < count &&
        (props[queueFamilyIndex].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)))
        *pSupported = VK_TRUE;
    return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities)
{
    struct vkl_surface *s = VKL_SURFACE(surface);
    VkSurfaceCapabilitiesKHR *c = pSurfaceCapabilities;
    LONG x, y, w, h;

    if (!s || s->magic != VKL_SURFACE_MAGIC || s->lost)
        return VK_ERROR_SURFACE_LOST_KHR;

    vkl_surface_rect(s, &x, &y, &w, &h);

    memset(c, 0, sizeof(*c));
    c->minImageCount = VKL_MIN_IMAGES;
    c->maxImageCount = VKL_MAX_IMAGES;
    c->currentExtent.width = w;
    c->currentExtent.height = h;
    c->minImageExtent = c->currentExtent;
    c->maxImageExtent = c->currentExtent;
    c->maxImageArrayLayers = 1;
    c->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    c->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    c->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    c->supportedUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                             VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    return VK_SUCCESS;
}

/* the formats WritePixelArray takes as they are */
static const struct { VkFormat format; ULONG rectfmt; } vkl_formats[] =
{
    { VK_FORMAT_B8G8R8A8_UNORM, RECTFMT_BGRA32 },
    { VK_FORMAT_B8G8R8A8_SRGB,  RECTFMT_BGRA32 },
    { VK_FORMAT_R8G8B8A8_UNORM, RECTFMT_RGBA32 },
    { VK_FORMAT_R8G8B8A8_SRGB,  RECTFMT_RGBA32 },
};
#define VKL_NUM_FORMATS (sizeof(vkl_formats) / sizeof(vkl_formats[0]))

static BOOL vkl_format_usable(VkPhysicalDevice phys, VkFormat format)
{
    struct vkl_icd_instance *ii = VKL_PHYSDEV_ICD(phys);
    VkFormatProperties props;

    memset(&props, 0, sizeof(props));
    ii->disp.GetPhysicalDeviceFormatProperties(phys, format, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats)
{
    VkSurfaceFormatKHR formats[VKL_NUM_FORMATS];
    uint32_t n = 0, i;
    VkResult res = VK_SUCCESS;

    for (i = 0; i < VKL_NUM_FORMATS; i++)
    {
        if (vkl_format_usable(physicalDevice, vkl_formats[i].format))
        {
            formats[n].format = vkl_formats[i].format;
            formats[n].colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            n++;
        }
    }

    if (!pSurfaceFormats)
        *pSurfaceFormatCount = n;
    else
    {
        if (*pSurfaceFormatCount < n)
            res = VK_INCOMPLETE;
        else
            *pSurfaceFormatCount = n;
        memcpy(pSurfaceFormats, formats, *pSurfaceFormatCount * sizeof(formats[0]));
    }
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
    static const VkPresentModeKHR modes[] = { VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR };
    uint32_t n = sizeof(modes) / sizeof(modes[0]);
    VkResult res = VK_SUCCESS;

    if (!pPresentModes)
        *pPresentModeCount = n;
    else
    {
        if (*pPresentModeCount < n)
            res = VK_INCOMPLETE;
        else
            *pPresentModeCount = n;
        memcpy(pPresentModes, modes, *pPresentModeCount * sizeof(modes[0]));
    }
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects)
{
    struct vkl_surface *s = VKL_SURFACE(surface);
    LONG x, y, w, h;

    if (!s || s->magic != VKL_SURFACE_MAGIC)
        return VK_ERROR_SURFACE_LOST_KHR;
    if (!pRects)
    {
        *pRectCount = 1;
        return VK_SUCCESS;
    }
    if (*pRectCount < 1)
        return VK_INCOMPLETE;
    vkl_surface_rect(s, &x, &y, &w, &h);
    *pRectCount = 1;
    pRects[0].offset.x = 0;
    pRects[0].offset.y = 0;
    pRects[0].extent.width = w;
    pRects[0].extent.height = h;
    return VK_SUCCESS;
}

/* VK_KHR_get_surface_capabilities2: the same answers in extensible structures */

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities)
{
    return vkl_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pSurfaceInfo->surface,
                                                      &pSurfaceCapabilities->surfaceCapabilities);
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount,
    VkSurfaceFormat2KHR *pSurfaceFormats)
{
    VkSurfaceFormatKHR formats[VKL_NUM_FORMATS];
    uint32_t n = VKL_NUM_FORMATS, i;
    VkResult res = VK_SUCCESS;

    vkl_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pSurfaceInfo->surface, &n, formats);

    if (!pSurfaceFormats)
    {
        *pSurfaceFormatCount = n;
        return VK_SUCCESS;
    }
    if (*pSurfaceFormatCount < n)
        res = VK_INCOMPLETE;
    else
        *pSurfaceFormatCount = n;
    for (i = 0; i < *pSurfaceFormatCount; i++)
        pSurfaceFormats[i].surfaceFormat = formats[i];
    return res;
}

void vkl_wsi_install_physdev(struct vkl_icd_instance *ii)
{
    /* the instance entry: non-NULL marks the command as implemented */
    ii->disp.DestroySurfaceKHR = vkDestroySurfaceKHR;
    ii->disp.GetPhysicalDeviceSurfaceSupportKHR = vkl_GetPhysicalDeviceSurfaceSupportKHR;
    ii->disp.GetPhysicalDeviceSurfaceCapabilitiesKHR = vkl_GetPhysicalDeviceSurfaceCapabilitiesKHR;
    ii->disp.GetPhysicalDeviceSurfaceFormatsKHR = vkl_GetPhysicalDeviceSurfaceFormatsKHR;
    ii->disp.GetPhysicalDeviceSurfacePresentModesKHR = vkl_GetPhysicalDeviceSurfacePresentModesKHR;
    ii->disp.GetPhysicalDevicePresentRectanglesKHR = vkl_GetPhysicalDevicePresentRectanglesKHR;
    ii->disp.GetPhysicalDeviceSurfaceCapabilities2KHR = vkl_GetPhysicalDeviceSurfaceCapabilities2KHR;
    ii->disp.GetPhysicalDeviceSurfaceFormats2KHR = vkl_GetPhysicalDeviceSurfaceFormats2KHR;
}

/* ---- swapchains ---- */

static BOOL vkl_memory_type(struct vkl_device *dev, uint32_t typeBits, VkMemoryPropertyFlags required,
    VkMemoryPropertyFlags preferred, uint32_t *indexp, VkMemoryPropertyFlags *flagsp)
{
    VkPhysicalDeviceMemoryProperties mp;
    uint32_t i;
    int pass;

    memset(&mp, 0, sizeof(mp));
    dev->icd->disp.GetPhysicalDeviceMemoryProperties(dev->phys, &mp);

    for (pass = 0; pass < 2; pass++)
    {
        VkMemoryPropertyFlags want = required | (pass == 0 ? preferred : 0);

        for (i = 0; i < mp.memoryTypeCount; i++)
        {
            if ((typeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want)
            {
                *indexp = i;
                if (flagsp)
                    *flagsp = mp.memoryTypes[i].propertyFlags;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void vkl_swapchain_free(struct vkl_swapchain *sc, const VkAllocationCallbacks *alloc)
{
    struct vkl_device *dev = sc->dev;
    VkDevice d = dev->handle;
    uint32_t i;

    if (sc->flip)
        vkl_flip_destroy(sc, alloc);
    if (sc->surface && sc->surface->swapchain == sc)
        sc->surface->swapchain = NULL;

    if (sc->pool)
        dev->drv.DestroyCommandPool(d, sc->pool, NULL);     /* frees the command buffers */
    if (sc->fence)
        dev->drv.DestroyFence(d, sc->fence, NULL);
    if (sc->pixels)
        dev->drv.UnmapMemory(d, sc->bufmem);
    if (sc->buffer)
        dev->drv.DestroyBuffer(d, sc->buffer, NULL);
    if (sc->bufmem)
        dev->drv.FreeMemory(d, sc->bufmem, NULL);
    for (i = 0; i < sc->imageCount; i++)
    {
        if (sc->images[i].image)
            dev->drv.DestroyImage(d, sc->images[i].image, alloc);
        if (sc->images[i].memory)
            dev->drv.FreeMemory(d, sc->images[i].memory, alloc);
    }
    sc->magic = 0;
    vkl_free(sc);
}

/* the command pool, for the family of the queue presenting */
static VkResult vkl_swapchain_pool(struct vkl_swapchain *sc, uint32_t family)
{
    struct vkl_device *dev = sc->dev;
    VkCommandPoolCreateInfo pi =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = family,
    };
    VkCommandBufferAllocateInfo ai =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkResult res;
    uint32_t i;

    if (sc->pool && sc->poolFamily == family)
        return VK_SUCCESS;

    if (sc->pool)
    {
        dev->drv.DestroyCommandPool(dev->handle, sc->pool, NULL);
        sc->pool = VK_NULL_HANDLE;
        for (i = 0; i < sc->imageCount; i++)
            sc->images[i].cmd = VK_NULL_HANDLE;
    }

    res = dev->drv.CreateCommandPool(dev->handle, &pi, NULL, &sc->pool);
    if (res != VK_SUCCESS)
        return res;
    sc->poolFamily = family;

    ai.commandPool = sc->pool;
    for (i = 0; i < sc->imageCount; i++)
    {
        res = dev->drv.AllocateCommandBuffers(dev->handle, &ai, &sc->images[i].cmd);
        if (res != VK_SUCCESS)
            return res;
    }
    return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_CreateSwapchainKHR(VkDevice device,
    const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
    VkSwapchainKHR *pSwapchain)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    struct vkl_surface *s = VKL_SURFACE(pCreateInfo->surface);
    struct vkl_swapchain *sc, *old = VKL_SWAPCHAIN(pCreateInfo->oldSwapchain);
    VkImageCreateInfo ici =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = pCreateInfo->imageFormat,
        .extent = { pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = pCreateInfo->imageUsage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = pCreateInfo->imageSharingMode,
        .queueFamilyIndexCount = pCreateInfo->queueFamilyIndexCount,
        .pQueueFamilyIndices = pCreateInfo->pQueueFamilyIndices,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkBufferCreateInfo bci =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    VkMemoryRequirements mr;
    VkMemoryPropertyFlags flags = 0;
    VkImageFormatListCreateInfo formatList;
    const VkBaseInStructure *ext;
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;
    uint32_t i, count;

    if (!s || s->magic != VKL_SURFACE_MAGIC || s->lost)
        return VK_ERROR_SURFACE_LOST_KHR;
    if (s->swapchain && s->swapchain != old)
        return VK_ERROR_NATIVE_WINDOW_IN_USE_KHR;
    if (pCreateInfo->imageArrayLayers != 1 || !pCreateInfo->imageExtent.width || !pCreateInfo->imageExtent.height)
        return VK_ERROR_INITIALIZATION_FAILED;

    sc = vkl_alloc(sizeof(*sc));
    if (!sc)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    sc->magic = VKL_SWAPCHAIN_MAGIC;
    sc->dev = dev;
    sc->format = pCreateInfo->imageFormat;
    sc->width = pCreateInfo->imageExtent.width;
    sc->height = pCreateInfo->imageExtent.height;
    sc->presentMode = pCreateInfo->presentMode;

    for (i = 0; i < VKL_NUM_FORMATS; i++)
        if (vkl_formats[i].format == sc->format)
            break;
    if (i == VKL_NUM_FORMATS)
        goto fail;
    sc->rectfmt = vkl_formats[i].rectfmt;

    /* a view of the images in another format needs the image to allow it */
    if (pCreateInfo->flags & VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR)
    {
        ici.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        for (ext = pCreateInfo->pNext; ext; ext = ext->pNext)
            if (ext->sType == VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO)
                break;
        if (ext)
        {
            formatList = *(const VkImageFormatListCreateInfo *)ext;
            formatList.pNext = NULL;
            ici.pNext = &formatList;
        }
    }

    /* the old swapchain leaves the display before the new one goes on it */
    if (old && old->magic == VKL_SWAPCHAIN_MAGIC)
        vkl_flip_detach(old);

    /* by scan-out when the display and the driver can, by read-back otherwise */
    sc->surface = s;
    res = vkl_flip_create(sc, pCreateInfo, &ici, pAllocator);
    sc->surface = NULL;
    if (res == VK_SUCCESS)
    {
        res = dev->drv.CreateFence(device, &fci, NULL, &sc->fence);
        if (res != VK_SUCCESS)
            goto fail;
        goto done;
    }

    count = pCreateInfo->minImageCount;
    if (count < VKL_MIN_IMAGES)
        count = VKL_MIN_IMAGES;
    if (count > VKL_MAX_IMAGES)
        count = VKL_MAX_IMAGES;

    for (i = 0; i < count; i++)
    {
        struct vkl_image *img = &sc->images[i];

        res = dev->drv.CreateImage(device, &ici, pAllocator, &img->image);
        if (res != VK_SUCCESS)
            goto fail;
        sc->imageCount = i + 1;

        dev->drv.GetImageMemoryRequirements(device, img->image, &mr);
        mai.allocationSize = mr.size;
        if (!vkl_memory_type(dev, mr.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             &mai.memoryTypeIndex, NULL))
        {
            res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
            goto fail;
        }
        res = dev->drv.AllocateMemory(device, &mai, pAllocator, &img->memory);
        if (res != VK_SUCCESS)
            goto fail;
        res = dev->drv.BindImageMemory(device, img->image, img->memory, 0);
        if (res != VK_SUCCESS)
            goto fail;
    }

    /* the read-back buffer */
    sc->bufsize = (VkDeviceSize)sc->width * sc->height * 4;
    bci.size = sc->bufsize;
    res = dev->drv.CreateBuffer(device, &bci, NULL, &sc->buffer);
    if (res != VK_SUCCESS)
        goto fail;
    dev->drv.GetBufferMemoryRequirements(device, sc->buffer, &mr);
    mai.allocationSize = mr.size;
    if (!vkl_memory_type(dev, mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                         VK_MEMORY_PROPERTY_HOST_CACHED_BIT, &mai.memoryTypeIndex, &flags))
    {
        res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
        goto fail;
    }
    sc->coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    res = dev->drv.AllocateMemory(device, &mai, NULL, &sc->bufmem);
    if (res != VK_SUCCESS)
        goto fail;
    res = dev->drv.BindBufferMemory(device, sc->buffer, sc->bufmem, 0);
    if (res != VK_SUCCESS)
        goto fail;
    res = dev->drv.MapMemory(device, sc->bufmem, 0, VK_WHOLE_SIZE, 0, &sc->pixels);
    if (res != VK_SUCCESS)
        goto fail;

    res = dev->drv.CreateFence(device, &fci, NULL, &sc->fence);
    if (res != VK_SUCCESS)
        goto fail;
    res = vkl_swapchain_pool(sc, dev->sigfamily);
    if (res != VK_SUCCESS)
        goto fail;

done:
    /* the surface is this swapchain's now */
    if (old && old->magic == VKL_SWAPCHAIN_MAGIC)
    {
        old->retired = TRUE;
        old->surface = NULL;
    }
    sc->surface = s;
    s->swapchain = sc;

    *pSwapchain = (VkSwapchainKHR)(IPTR)sc;
    return VK_SUCCESS;

fail:
    vkl_swapchain_free(sc, pAllocator);
    return res;
}

static VKAPI_ATTR void VKAPI_CALL vkl_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
    const VkAllocationCallbacks *pAllocator)
{
    struct vkl_swapchain *sc = VKL_SWAPCHAIN(swapchain);

    if (!sc || sc->magic != VKL_SWAPCHAIN_MAGIC)
        return;
    vkl_swapchain_free(sc, pAllocator);
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain,
    uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages)
{
    struct vkl_swapchain *sc = VKL_SWAPCHAIN(swapchain);
    VkResult res = VK_SUCCESS;
    uint32_t i;

    if (!pSwapchainImages)
    {
        *pSwapchainImageCount = sc->imageCount;
        return VK_SUCCESS;
    }
    if (*pSwapchainImageCount < sc->imageCount)
        res = VK_INCOMPLETE;
    else
        *pSwapchainImageCount = sc->imageCount;
    for (i = 0; i < *pSwapchainImageCount; i++)
        pSwapchainImages[i] = sc->images[i].image;
    return res;
}

/* does the swapchain still fit its surface? */
static BOOL vkl_swapchain_current(struct vkl_swapchain *sc)
{
    LONG x, y, w, h;

    vkl_surface_rect(sc->surface, &x, &y, &w, &h);
    return (uint32_t)w == sc->width && (uint32_t)h == sc->height;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain,
    uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    struct vkl_swapchain *sc = VKL_SWAPCHAIN(swapchain);
    VkResult res = VK_SUCCESS;
    uint32_t i, idx = 0;

    if (!sc || sc->magic != VKL_SWAPCHAIN_MAGIC)
        return VK_ERROR_OUT_OF_DATE_KHR;
    if (sc->retired || !sc->surface)
        return VK_ERROR_OUT_OF_DATE_KHR;
    if (sc->surface->lost)
        return VK_ERROR_SURFACE_LOST_KHR;
    if (!vkl_swapchain_current(sc))
        return VK_ERROR_OUT_OF_DATE_KHR;

    if (sc->flip)
    {
        /* the one buffer the display has released */
        if (!vkl_flip_next(sc, &idx))
            return timeout ? VK_TIMEOUT : VK_NOT_READY;
        i = 0;
    }
    else
    {
        for (i = 0; i < sc->imageCount; i++)
        {
            idx = (sc->next + i) % sc->imageCount;
            if (!sc->images[idx].acquired)
                break;
        }
    }
    if (i == sc->imageCount)
        return timeout ? VK_TIMEOUT : VK_NOT_READY;     /* every image is the application's */

    /* the image is free now: signal what the application waits on */
    if (semaphore || fence)
    {
        VkSubmitInfo si =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .signalSemaphoreCount = semaphore ? 1 : 0,
            .pSignalSemaphores = &semaphore,
        };

        if (!dev->sigqueue)
            return VK_ERROR_INITIALIZATION_FAILED;
        res = vkl_queue_submit(dev, dev->sigqueue, 1, &si, fence);
        if (res != VK_SUCCESS)
            return res;
    }

    sc->images[idx].acquired = TRUE;
    sc->next = (idx + 1) % sc->imageCount;
    *pImageIndex = idx;
    return VK_SUCCESS;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_AcquireNextImage2KHR(VkDevice device,
    const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex)
{
    return vkl_AcquireNextImageKHR(device, pAcquireInfo->swapchain, pAcquireInfo->timeout,
                                   pAcquireInfo->semaphore, pAcquireInfo->fence, pImageIndex);
}

/*
 * Present one image: copy it into the read-back buffer on the presenting
 * queue (after the semaphores the application names), wait for the copy and
 * write the pixels into the window.
 */
static VkResult vkl_present_one(struct vkl_device *dev, VkQueue queue, struct vkl_swapchain *sc,
    uint32_t index, uint32_t waitCount, const VkSemaphore *waits)
{
    VkDevice d = dev->handle;
    struct vkl_image *img;
    VkPipelineStageFlags stages[16];
    VkCommandBufferBeginInfo bi =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkBufferImageCopy region =
    {
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
    };
    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkResult res;
    uint32_t i;
    LONG x, y, w, h;
    BOOL current;

    if (!sc || sc->magic != VKL_SWAPCHAIN_MAGIC || index >= sc->imageCount)
        return VK_ERROR_OUT_OF_DATE_KHR;
    if (sc->flip)
        return vkl_flip_present(dev, queue, sc, index, waitCount, waits);
    img = &sc->images[index];

    if (waitCount > 16)
        waitCount = 16;
    for (i = 0; i < waitCount; i++)
        stages[i] = VK_PIPELINE_STAGE_TRANSFER_BIT;

    res = vkl_swapchain_pool(sc, vkl_queue_family(dev, queue));
    if (res != VK_SUCCESS)
        return res;

    dev->drv.ResetCommandBuffer(img->cmd, 0);
    res = dev->drv.BeginCommandBuffer(img->cmd, &bi);
    if (res != VK_SUCCESS)
        return res;

    barrier.image = img->image;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    dev->drv.CmdPipelineBarrier(img->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, NULL, 0, NULL, 1, &barrier);

    region.imageExtent.width = sc->width;
    region.imageExtent.height = sc->height;
    region.imageExtent.depth = 1;
    dev->drv.CmdCopyImageToBuffer(img->cmd, img->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  sc->buffer, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = 0;
    dev->drv.CmdPipelineBarrier(img->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, NULL, 0, NULL, 1, &barrier);

    res = dev->drv.EndCommandBuffer(img->cmd);
    if (res != VK_SUCCESS)
        return res;

    si.waitSemaphoreCount = waitCount;
    si.pWaitSemaphores = waits;
    si.pWaitDstStageMask = stages;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &img->cmd;
    res = vkl_queue_submit(dev, queue, 1, &si, sc->fence);
    if (res != VK_SUCCESS)
        return res;

    res = dev->drv.WaitForFences(d, 1, &sc->fence, VK_TRUE, UINT64_MAX);
    dev->drv.ResetFences(d, 1, &sc->fence);
    img->acquired = FALSE;
    if (res != VK_SUCCESS)
        return res;

    if (sc->retired || !sc->surface)
        return VK_ERROR_OUT_OF_DATE_KHR;
    if (sc->surface->lost)
        return VK_ERROR_SURFACE_LOST_KHR;

    if (!sc->coherent)
    {
        VkMappedMemoryRange range =
        {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = sc->bufmem,
            .size = VK_WHOLE_SIZE,
        };

        dev->drv.InvalidateMappedMemoryRanges(d, 1, &range);
    }

    vkl_surface_rect(sc->surface, &x, &y, &w, &h);
    current = ((uint32_t)w == sc->width && (uint32_t)h == sc->height);
    if ((uint32_t)w > sc->width)
        w = sc->width;
    if ((uint32_t)h > sc->height)
        h = sc->height;

    if (sc->presentMode != VK_PRESENT_MODE_IMMEDIATE_KHR)
        WaitTOF();
    WritePixelArray(sc->pixels, 0, 0, sc->width * 4, sc->surface->window->RPort, x, y, w, h, sc->rectfmt);
    sc->surface->presentPath = VKPP_Readback;

    return current ? VK_SUCCESS : VK_SUBOPTIMAL_KHR;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    struct vkl_device *dev = VKL_DEVICE(queue);
    VkResult result = VK_SUCCESS;
    uint32_t i;

    for (i = 0; i < pPresentInfo->swapchainCount; i++)
    {
        /* the semaphores are waited for once, by the first copy */
        VkResult res = vkl_present_one(dev, queue, VKL_SWAPCHAIN(pPresentInfo->pSwapchains[i]),
            pPresentInfo->pImageIndices[i],
            i == 0 ? pPresentInfo->waitSemaphoreCount : 0, pPresentInfo->pWaitSemaphores);

        if (pPresentInfo->pResults)
            pPresentInfo->pResults[i] = res;
        /* an error wins over suboptimal, suboptimal over success */
        if (res < 0 && result >= 0)
            result = res;
        else if (res > 0 && result == VK_SUCCESS)
            result = res;
    }
    return result;
}

/*
 * Names and tags for the library's own objects are not the driver's
 * business (it would take the handle for one of its own).
 */
static BOOL vkl_is_own_object(VkObjectType type)
{
    return type == VK_OBJECT_TYPE_SURFACE_KHR || type == VK_OBJECT_TYPE_SWAPCHAIN_KHR;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_SetDebugUtilsObjectNameEXT(VkDevice device,
    const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    VkDebugUtilsObjectNameInfoEXT info = *pNameInfo;

    if (vkl_is_own_object(info.objectType))
        return VK_SUCCESS;
    if (info.objectType == VK_OBJECT_TYPE_INSTANCE)
        info.objectHandle = (uint64_t)(IPTR)dev->icd->handle;
    return dev->drv.SetDebugUtilsObjectNameEXT(device, &info);
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_SetDebugUtilsObjectTagEXT(VkDevice device,
    const VkDebugUtilsObjectTagInfoEXT *pTagInfo)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    VkDebugUtilsObjectTagInfoEXT info = *pTagInfo;

    if (vkl_is_own_object(info.objectType))
        return VK_SUCCESS;
    if (info.objectType == VK_OBJECT_TYPE_INSTANCE)
        info.objectHandle = (uint64_t)(IPTR)dev->icd->handle;
    return dev->drv.SetDebugUtilsObjectTagEXT(device, &info);
}

void vkl_wsi_install_device(struct vkl_device *dev)
{
    dev->disp.CreateSwapchainKHR = vkl_CreateSwapchainKHR;
    dev->disp.DestroySwapchainKHR = vkl_DestroySwapchainKHR;
    dev->disp.GetSwapchainImagesKHR = vkl_GetSwapchainImagesKHR;
    dev->disp.AcquireNextImageKHR = vkl_AcquireNextImageKHR;
    dev->disp.AcquireNextImage2KHR = vkl_AcquireNextImage2KHR;
    dev->disp.QueuePresentKHR = vkl_QueuePresentKHR;

    if (dev->drv.SetDebugUtilsObjectNameEXT)
        dev->disp.SetDebugUtilsObjectNameEXT = vkl_SetDebugUtilsObjectNameEXT;
    if (dev->drv.SetDebugUtilsObjectTagEXT)
        dev->disp.SetDebugUtilsObjectTagEXT = vkl_SetDebugUtilsObjectTagEXT;
}

/* ---- vkGetInstanceProcAddr's share ---- */

PFN_vkVoidFunction vkl_wsi_instance_proc(struct vkl_instance *inst, const char *name)
{
    static const char * const device_cmds[] =
    {
        "vkCreateSwapchainKHR", "vkDestroySwapchainKHR", "vkGetSwapchainImagesKHR",
        "vkAcquireNextImageKHR", "vkAcquireNextImage2KHR", "vkQueuePresentKHR",
    };
    ULONG i;

    if (!strcmp(name, "vkCreateAROSSurfaceAROS"))
        return (PFN_vkVoidFunction)vkCreateAROSSurfaceAROS;
    if (!strcmp(name, "vkACreateSurface"))
        return (PFN_vkVoidFunction)vkACreateSurface;
    if (!strcmp(name, "vkAGetSurfaceAttr"))
        return (PFN_vkVoidFunction)vkAGetSurfaceAttr;

    for (i = 0; i < sizeof(device_cmds) / sizeof(device_cmds[0]); i++)
    {
        if (!strcmp(name, device_cmds[i]))
        {
            const struct vkl_command *cmd = vkl_find_command(name);

            return cmd ? cmd->entry : NULL;
        }
    }
    return NULL;
}
