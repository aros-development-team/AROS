/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <utility/tagitem.h>
#include <proto/alib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_aros.h>

/* The varargs version of vulkan.library/vkACreateSurface() */
VkResult vkACreateSurfaceTags(VkInstance instance, const VkAllocationCallbacks *pAllocator,
    VkSurfaceKHR *pSurface, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, VkResult)
    retval = vkACreateSurface(instance, AROS_SLOWSTACKTAGS_ARG(tag1), pAllocator, pSurface);
    AROS_SLOWSTACKTAGS_POST
}
