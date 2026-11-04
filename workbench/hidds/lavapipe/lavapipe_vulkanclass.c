/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    hidd.vulkan.lavapipe: Mesa's lavapipe (Vulkan on llvmpipe) behind the
    Vulkan HIDD interface. The ICD's vk_icdGetInstanceProcAddr serves every
    entry point; the base class implements the other methods through it.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_icd.h>

#include "vk_device.h"

#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/vulkan_hidd.h>

#include "lavapipe_intern.h"

#include <string.h>

#define UtilityBase    (&BASE(cl->UserData)->sd)->UtilityBase

#undef HiddVulkanAttrBase
#define HiddVulkanAttrBase   (SD(cl)->hiddVulkanAB)

/* lavapipe's ICD entry point (lvp_device.c) */
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName);

OOP_Object *METHOD(HiddLavapipe, Root, New)
{
    IPTR interfaceVers;

    D(bug("[Lavapipe] %s()\n", __PRETTY_FUNCTION__));

    interfaceVers = GetTagData(aHidd_Vulkan_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != VULKAN_INTERFACE_VERSION)
    {
        D(bug("[Lavapipe] %s: interface version mismatch (got %ld, want %d)\n",
              __PRETTY_FUNCTION__, (long)interfaceVers, VULKAN_INTERFACE_VERSION));
        return NULL;
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddVulkanLavapipeData *data = OOP_INST_DATA(cl, o);

        /*
         * Nothing to bring up here: the instance is made on the first
         * vkCreateInstance through the entry points below, and llvmpipe
         * (and with it LLVM) on the first physical device enumeration.
         */
        data->lavapipe_obj = o;
    }
    else
    {
        D(bug("[Lavapipe] %s: OOP_DoSuperMethod returned NULL\n", __PRETTY_FUNCTION__));
    }

    return o;
}

VOID METHOD(HiddLavapipe, Root, Dispose)
{
    D(bug("[Lavapipe] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddLavapipe, Root, Get)
{
    ULONG idx;

    D(bug("[Lavapipe] %s()\n", __PRETTY_FUNCTION__));

    if (IS_VULKAN_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Vulkan_InterfaceVersion:
                *msg->storage = VULKAN_INTERFACE_VERSION;
                return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * Layers are the loader's business, and there is none here: no layer, so
 * a count of 0 whether or not an array is passed. lavapipe's own
 * implementation answers a non-NULL array with VK_ERROR_LAYER_NOT_PRESENT,
 * which a client that asks for the count and then fills an array (zink
 * does) takes as a failure.
 */
static VKAPI_ATTR VkResult VKAPI_CALL
lavapipe_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    if (pPropertyCount)
        *pPropertyCount = 0;
    return VK_SUCCESS;
}

/*
 * The ICD's vkGetInstanceProcAddr. With a NULL instance the global
 * commands (vkCreateInstance, vkEnumerateInstance*) come back, and the
 * name "vkGetInstanceProcAddr" the entry point itself.
 */
APTR METHOD(HiddLavapipe, Hidd_Vulkan, GetInstanceProcAddr)
{
    D(bug("[Lavapipe] %s(%p, '%s')\n", __PRETTY_FUNCTION__, msg->instance, msg->pName ? msg->pName : "(null)"));

    if (!msg->pName)
        return NULL;

    if (!strcmp(msg->pName, "vkGetInstanceProcAddr"))
        return (APTR)vk_icdGetInstanceProcAddr;

    if (!strcmp(msg->pName, "vkEnumerateInstanceLayerProperties"))
        return (APTR)lavapipe_EnumerateInstanceLayerProperties;

    return (APTR)vk_icdGetInstanceProcAddr((VkInstance)msg->instance, msg->pName);
}

/* The device's dispatch table, as the runtime's vkGetDeviceProcAddr reads it */
APTR METHOD(HiddLavapipe, Hidd_Vulkan, GetDeviceProcAddr)
{
    VK_FROM_HANDLE(vk_device, device, (VkDevice)msg->device);

    D(bug("[Lavapipe] %s(%p, '%s')\n", __PRETTY_FUNCTION__, msg->device, msg->pName ? msg->pName : "(null)"));

    if (!device || !msg->pName)
        return NULL;

    return (APTR)vk_device_get_proc_addr(device, msg->pName);
}
