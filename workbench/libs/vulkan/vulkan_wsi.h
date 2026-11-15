#ifndef VULKAN_WSI_H
#define VULKAN_WSI_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: surfaces and swapchains (vulkan_wsi.c, vulkan_flip.c).
*/

#include "vulkan_intern.h"

#define VKL_SURFACE_MAGIC   0x564B4C53      /* 'VKLS' */
#define VKL_SWAPCHAIN_MAGIC 0x564B4C43      /* 'VKLC' */

#define VKL_MIN_IMAGES      2
#define VKL_MAX_IMAGES      8

struct vkl_swapchain;

struct vkl_surface
{
    struct MinNode          node;
    ULONG                   magic;
    struct vkl_instance    *instance;
    struct Window          *window;
    BOOL                    subrect;        /* a rectangle of the inner area */
    LONG                    left, top, width, height;
    struct vkl_swapchain   *swapchain;      /* the current (not retired) one */
    ULONG                   presentPath;
    struct MinNode          allnode;        /* in the list of every surface (vkl_wsi_lock) */
    BOOL                    lost;           /* its window was closed */
};

struct vkl_image
{
    VkImage                 image;
    VkDeviceMemory          memory;
    VkCommandBuffer         cmd;
    BOOL                    acquired;
};

struct vkl_swapchain
{
    ULONG                   magic;
    struct vkl_device      *dev;
    struct vkl_surface     *surface;
    BOOL                    retired;        /* replaced, or its surface is gone */

    VkFormat                format;
    ULONG                   rectfmt;        /* RECTFMT_* of the format's bytes */
    uint32_t                width, height;
    VkPresentModeKHR        presentMode;

    uint32_t                imageCount;
    struct vkl_image        images[VKL_MAX_IMAGES];
    uint32_t                next;           /* round robin start for acquisition */

    /* the read-back buffer, shared by the images: presentation is serial */
    VkBuffer                buffer;
    VkDeviceMemory          bufmem;
    APTR                    pixels;
    VkDeviceSize            bufsize;
    BOOL                    coherent;

    VkCommandPool           pool;
    uint32_t                poolFamily;
    VkFence                 fence;

    struct vkl_flip        *flip;           /* presenting by scan-out; NULL: by read-back */
};

#define VKL_SURFACE(handle)     ((struct vkl_surface *)(IPTR)(handle))
#define VKL_SWAPCHAIN(handle)   ((struct vkl_swapchain *)(IPTR)(handle))

/* every surface there is, and the scan-out state of their swapchains */
extern struct SignalSemaphore vkl_wsi_lock;
extern struct MinList vkl_wsi_surfaces;

void vkl_surface_rect(struct vkl_surface *s, LONG *x, LONG *y, LONG *w, LONG *h);

/* ---- presenting by scan-out (vulkan_flip.c) ---- */

void vkl_flip_init(void);
void vkl_flip_exit(void);

/* the driver extensions the scan-out path imports display buffers with */
#define VKL_FLIP_NUM_EXTS 3
extern const char * const vkl_flip_exts[VKL_FLIP_NUM_EXTS];

/*
 * Make the swapchain's images from a scan-out bitmap of the window's
 * display and attach it to the window. VK_SUCCESS, or anything else when
 * the display, the driver or the format do not allow it: the caller then
 * makes a read-back swapchain. ici is the image description the read-back
 * path would use.
 */
VkResult vkl_flip_create(struct vkl_swapchain *sc, const VkSwapchainCreateInfoKHR *info,
                         const VkImageCreateInfo *ici, const VkAllocationCallbacks *alloc);
/* take the surface off the display; the images stay valid */
void vkl_flip_detach(struct vkl_swapchain *sc);
/* detach, and free the images and the bitmap */
void vkl_flip_destroy(struct vkl_swapchain *sc, const VkAllocationCallbacks *alloc);
/* the image to render next, or FALSE when the application still has it */
BOOL vkl_flip_next(struct vkl_swapchain *sc, uint32_t *index);
VkResult vkl_flip_present(struct vkl_device *dev, VkQueue queue, struct vkl_swapchain *sc,
                          uint32_t index, uint32_t waitCount, const VkSemaphore *waits);
IPTR vkl_flip_attr(struct vkl_swapchain *sc, ULONG attr);

#endif /* VULKAN_WSI_H */
