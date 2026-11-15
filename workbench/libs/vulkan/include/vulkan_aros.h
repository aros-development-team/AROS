#ifndef VULKAN_AROS_H_
#define VULKAN_AROS_H_ 1

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AROS window system integration for Vulkan (vulkan.library).

    A surface is made from an Intuition window, either through the
    VK_AROS_surface instance extension (vkCreateAROSSurfaceAROS), the form
    ported code expects, or through the tag based vkACreateSurface().

    VK_AROS_surface is not registered with Khronos yet: the extension
    number and the structure type below are PROVISIONAL and will change
    when it is. Use the names, never the values.
*/

#include <exec/types.h>
#include <utility/tagitem.h>
#include <vulkan/vulkan_core.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Window;

/* ---- VK_AROS_surface (provisional numbering) ---- */

#define VK_AROS_surface 1
#define VK_AROS_SURFACE_SPEC_VERSION        1
#define VK_AROS_SURFACE_EXTENSION_NAME      "VK_AROS_surface"

#define VK_AROS_SURFACE_EXTENSION_NUMBER    901     /* provisional */
#define VK_STRUCTURE_TYPE_AROS_SURFACE_CREATE_INFO_AROS \
    ((VkStructureType)(1000000000 + (VK_AROS_SURFACE_EXTENSION_NUMBER - 1) * 1000))

typedef VkFlags VkAROSSurfaceCreateFlagsAROS;

typedef struct VkAROSSurfaceCreateInfoAROS {
    VkStructureType                 sType;
    const void                     *pNext;
    VkAROSSurfaceCreateFlagsAROS    flags;
    struct Window                  *window;     /* the surface is its inner area */
} VkAROSSurfaceCreateInfoAROS;

typedef VkResult (VKAPI_PTR *PFN_vkCreateAROSSurfaceAROS)(VkInstance instance,
    const VkAROSSurfaceCreateInfoAROS *pCreateInfo, const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface);

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkCreateAROSSurfaceAROS(VkInstance instance,
    const VkAROSSurfaceCreateInfoAROS *pCreateInfo, const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface);
#endif

/* ---- the native calls ---- */

/* vkACreateSurface() tags */
#define VKA_Dummy       (TAG_USER + 0x564B0000)
#define VKA_Window      (VKA_Dummy + 1)     /* struct Window *, required */
/* a rectangle of the window instead of its whole inner area, relative to
   the inner area's top left corner; all four, or none */
#define VKA_Left        (VKA_Dummy + 2)
#define VKA_Top         (VKA_Dummy + 3)
#define VKA_Width       (VKA_Dummy + 4)
#define VKA_Height      (VKA_Dummy + 5)

/* vkAGetSurfaceAttr() attributes */
#define VKSA_Window     1   /* struct Window * */
#define VKSA_Width      2   /* the surface's current size */
#define VKSA_Height     3
#define VKSA_PresentPath 4  /* VKPP_*: how the surface's swapchain last presented */

#define VKSA_OnPlane    5   /* BOOL: a hardware plane shows the swapchain right now */
#define VKSA_ScanoutBitMap 6 /* OOP_Object *: the swapchain's scan-out bitmap, for diagnostics */
#define VKSA_Buffers    7   /* buffers of that bitmap */

#define VKPP_None       0
#define VKPP_Readback   1   /* copied through the CPU into the window */
#define VKPP_Flip       2   /* the images are display buffers: shown by the display itself */

#ifndef VK_NO_PROTOTYPES
VKAPI_ATTR VkResult VKAPI_CALL vkACreateSurface(VkInstance instance, struct TagItem *tagList,
    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
VKAPI_ATTR IPTR VKAPI_CALL vkAGetSurfaceAttr(VkInstance instance, VkSurfaceKHR surface, ULONG attr);
/* the varargs form of vkACreateSurface(), in libvulkan.a */
VkResult vkACreateSurfaceTags(VkInstance instance, const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface, Tag tag1, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* VULKAN_AROS_H_ */
