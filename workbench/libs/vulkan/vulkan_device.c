/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: logical devices. A device's table is the driver's own
    entry points, except for the few the library has to see: the ones that
    hand out dispatchable objects (their loader word must be set), the
    queue commands of the queue the library submits on itself, and the
    swapchain.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <string.h>

#include "vulkan_wsi.h"

/* ---- physical device level ---- */

static VKAPI_ATTR VkResult VKAPI_CALL vkl_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
    uint32_t *pPropertyCount, VkLayerProperties *pProperties)
{
    if (pPropertyCount)
        *pPropertyCount = 0;
    return VK_SUCCESS;
}

/* the driver's device extensions without its own WSI, plus the library's */
static VKAPI_ATTR VkResult VKAPI_CALL vkl_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
    const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    struct vkl_icd_instance *ii = VKL_PHYSDEV_ICD(physicalDevice);
    PFN_vkEnumerateDeviceExtensionProperties drv =
        (PFN_vkEnumerateDeviceExtensionProperties)ii->icd->gipa(ii->handle, "vkEnumerateDeviceExtensionProperties");
    VkExtensionProperties *props;
    uint32_t count = 0, i, n = 0;
    VkResult res;

    if (pLayerName)
        return VK_ERROR_LAYER_NOT_PRESENT;
    if (!drv)
        return VK_ERROR_INITIALIZATION_FAILED;

    res = drv(physicalDevice, NULL, &count, NULL);
    if (res < 0)
        return res;
    props = vkl_alloc((count + 1) * sizeof(*props));
    if (!props)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    res = drv(physicalDevice, NULL, &count, props);
    if (res < 0)
    {
        vkl_free(props);
        return res;
    }
    for (i = 0; i < count; i++)
        if (!vkl_ext_hidden_device(props[i].extensionName))
            props[n++] = props[i];
    strcpy(props[n].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    props[n].specVersion = VK_KHR_SWAPCHAIN_SPEC_VERSION;
    n++;

    res = VK_SUCCESS;
    if (!pProperties)
        *pPropertyCount = n;
    else
    {
        if (*pPropertyCount < n)
            res = VK_INCOMPLETE;
        else
            *pPropertyCount = n;
        memcpy(pProperties, props, *pPropertyCount * sizeof(*props));
    }
    vkl_free(props);
    return res;
}

/* ---- queues ---- */

uint32_t vkl_queue_family(struct vkl_device *dev, VkQueue queue)
{
    ULONG i;

    for (i = 0; i < dev->queueCount; i++)
        if (dev->queues[i].queue == queue)
            return dev->queues[i].family;
    return dev->sigfamily;
}

static void vkl_queue_register(struct vkl_device *dev, VkQueue queue, uint32_t family)
{
    ULONG i;

    if (!queue)
        return;
    VKL_SET_DISPATCH(queue, &dev->disp);

    ObtainSemaphore(&dev->qlock);
    for (i = 0; i < dev->queueCount; i++)
        if (dev->queues[i].queue == queue)
            break;
    if (i == dev->queueCount && dev->queueCount < VKL_MAX_QUEUES)
    {
        dev->queues[i].queue = queue;
        dev->queues[i].family = family;
        dev->queueCount++;
    }
    ReleaseSemaphore(&dev->qlock);
}

static VKAPI_ATTR void VKAPI_CALL vkl_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex,
    uint32_t queueIndex, VkQueue *pQueue)
{
    struct vkl_device *dev = VKL_DEVICE(device);

    dev->drv.GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    vkl_queue_register(dev, *pQueue, queueFamilyIndex);
}

static VKAPI_ATTR void VKAPI_CALL vkl_GetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo,
    VkQueue *pQueue)
{
    struct vkl_device *dev = VKL_DEVICE(device);

    dev->drv.GetDeviceQueue2(device, pQueueInfo, pQueue);
    vkl_queue_register(dev, *pQueue, pQueueInfo->queueFamilyIndex);
}

/*
 * The queue the library signals acquired images on is also the
 * application's: its use of it goes through the lock. The other queues are
 * not touched.
 */
#define VKL_QLOCK(dev, queue)   BOOL locked = ((queue) == (dev)->sigqueue); if (locked) ObtainSemaphore(&(dev)->qlock)
#define VKL_QUNLOCK(dev)        if (locked) ReleaseSemaphore(&(dev)->qlock)

VkResult vkl_queue_submit(struct vkl_device *dev, VkQueue queue, uint32_t count,
                          const VkSubmitInfo *submits, VkFence fence)
{
    VkResult res;
    VKL_QLOCK(dev, queue);

    res = dev->drv.QueueSubmit(queue, count, submits, fence);
    VKL_QUNLOCK(dev);
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueueSubmit(VkQueue queue, uint32_t submitCount,
    const VkSubmitInfo *pSubmits, VkFence fence)
{
    return vkl_queue_submit(VKL_DEVICE(queue), queue, submitCount, pSubmits, fence);
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueueSubmit2(VkQueue queue, uint32_t submitCount,
    const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkl_device *dev = VKL_DEVICE(queue);
    VkResult res;
    VKL_QLOCK(dev, queue);

    res = dev->drv.QueueSubmit2(queue, submitCount, pSubmits, fence);
    VKL_QUNLOCK(dev);
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueueSubmit2KHR(VkQueue queue, uint32_t submitCount,
    const VkSubmitInfo2 *pSubmits, VkFence fence)
{
    struct vkl_device *dev = VKL_DEVICE(queue);
    VkResult res;
    VKL_QLOCK(dev, queue);

    res = dev->drv.QueueSubmit2KHR(queue, submitCount, pSubmits, fence);
    VKL_QUNLOCK(dev);
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount,
    const VkBindSparseInfo *pBindInfo, VkFence fence)
{
    struct vkl_device *dev = VKL_DEVICE(queue);
    VkResult res;
    VKL_QLOCK(dev, queue);

    res = dev->drv.QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    VKL_QUNLOCK(dev);
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_QueueWaitIdle(VkQueue queue)
{
    struct vkl_device *dev = VKL_DEVICE(queue);
    VkResult res;
    VKL_QLOCK(dev, queue);

    res = dev->drv.QueueWaitIdle(queue);
    VKL_QUNLOCK(dev);
    return res;
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_DeviceWaitIdle(VkDevice device)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    VkResult res;

    ObtainSemaphore(&dev->qlock);
    res = dev->drv.DeviceWaitIdle(device);
    ReleaseSemaphore(&dev->qlock);
    return res;
}

/* ---- command buffers ---- */

static VKAPI_ATTR VkResult VKAPI_CALL vkl_AllocateCommandBuffers(VkDevice device,
    const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers)
{
    struct vkl_device *dev = VKL_DEVICE(device);
    VkResult res = dev->drv.AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    uint32_t i;

    if (res == VK_SUCCESS)
        for (i = 0; i < pAllocateInfo->commandBufferCount; i++)
            if (pCommandBuffers[i])
                VKL_SET_DISPATCH(pCommandBuffers[i], &dev->disp);
    return res;
}

/* ---- device ---- */

static VKAPI_ATTR void VKAPI_CALL vkl_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
    struct vkl_device *dev;

    if (!device)
        return;
    dev = VKL_DEVICE(device);
    dev->drv.DestroyDevice(device, pAllocator);
    vkl_free(dev);
}

static VKAPI_ATTR VkResult VKAPI_CALL vkl_CreateDevice(VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice)
{
    struct vkl_icd_instance *ii = VKL_PHYSDEV_ICD(physicalDevice);
    PFN_vkCreateDevice create = (PFN_vkCreateDevice)ii->icd->gipa(ii->handle, "vkCreateDevice");
    VkDeviceCreateInfo info = *pCreateInfo;
    const char **names = NULL;
    struct vkl_device *dev;
    uint32_t i, enabled = 0;
    VkResult res;

    if (!create || !ii->gdpa)
        return VK_ERROR_INITIALIZATION_FAILED;

    dev = vkl_alloc(sizeof(*dev));
    if (!dev)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    dev->icd = ii;
    dev->phys = physicalDevice;
    InitSemaphore(&dev->qlock);

    names = vkl_alloc((pCreateInfo->enabledExtensionCount + VKL_FLIP_NUM_EXTS) * sizeof(*names));
    if (!names)
    {
        vkl_free(dev);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++)
    {
        const char *name = pCreateInfo->ppEnabledExtensionNames[i];

        if (!strcmp(name, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            dev->swapchain = TRUE;
        else if (vkl_ext_hidden_device(name))
        {
            vkl_free(names);
            vkl_free(dev);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        else
            names[enabled++] = name;
    }
    /* a swapchain can be display buffers when the driver imports them: its
       extensions for that, on top of the application's */
    if (dev->swapchain)
    {
        PFN_vkEnumerateDeviceExtensionProperties enumerate =
            (PFN_vkEnumerateDeviceExtensionProperties)ii->icd->gipa(ii->handle, "vkEnumerateDeviceExtensionProperties");
        VkExtensionProperties *props = NULL;
        uint32_t count = 0, j, have = 0;

        if (enumerate && enumerate(physicalDevice, NULL, &count, NULL) >= 0 && count &&
            (props = vkl_alloc(count * sizeof(*props))) &&
            enumerate(physicalDevice, NULL, &count, props) >= 0)
        {
            for (i = 0; i < VKL_FLIP_NUM_EXTS; i++)
                for (j = 0; j < count; j++)
                    if (!strcmp(props[j].extensionName, vkl_flip_exts[i]))
                    {
                        have++;
                        break;
                    }
        }
        vkl_free(props);

        if (have == VKL_FLIP_NUM_EXTS)
        {
            for (i = 0; i < VKL_FLIP_NUM_EXTS; i++)
            {
                for (j = 0; j < enabled; j++)
                    if (!strcmp(names[j], vkl_flip_exts[i]))
                        break;
                if (j == enabled)
                    names[enabled++] = vkl_flip_exts[i];
            }
            dev->flipExts = TRUE;
        }
    }

    info.enabledExtensionCount = enabled;
    info.ppEnabledExtensionNames = names;
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = NULL;

    res = create(physicalDevice, &info, pAllocator, &dev->handle);
    vkl_free(names);
    if (res != VK_SUCCESS)
    {
        vkl_free(dev);
        return res;
    }

    /* the driver's entry points ... */
    for (i = 0; i < VKL_NUM_COMMANDS; i++)
    {
        const struct vkl_command *cmd = &vkl_commands[i];

        if (cmd->level == VKL_LEVEL_DEVICE)
            VKL_SLOT(&dev->disp, cmd) = ii->gdpa(dev->handle, cmd->name);
    }
    dev->drv = dev->disp;

    /* ... and the library's */
    dev->disp.DestroyDevice = vkl_DestroyDevice;
    dev->disp.GetDeviceQueue = vkl_GetDeviceQueue;
    if (dev->drv.GetDeviceQueue2)
        dev->disp.GetDeviceQueue2 = vkl_GetDeviceQueue2;
    dev->disp.AllocateCommandBuffers = vkl_AllocateCommandBuffers;

    VKL_SET_DISPATCH(dev->handle, &dev->disp);

    if (dev->swapchain)
    {
        /* a queue to signal acquired images on: the first one made without
           flags, which GetDeviceQueue can name */
        for (i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
        {
            const VkDeviceQueueCreateInfo *q = &pCreateInfo->pQueueCreateInfos[i];

            if (q->flags == 0 && q->queueCount)
            {
                dev->sigfamily = q->queueFamilyIndex;
                dev->drv.GetDeviceQueue(dev->handle, q->queueFamilyIndex, 0, &dev->sigqueue);
                vkl_queue_register(dev, dev->sigqueue, q->queueFamilyIndex);
                break;
            }
        }

        dev->disp.QueueSubmit = vkl_QueueSubmit;
        if (dev->drv.QueueSubmit2)
            dev->disp.QueueSubmit2 = vkl_QueueSubmit2;
        if (dev->drv.QueueSubmit2KHR)
            dev->disp.QueueSubmit2KHR = vkl_QueueSubmit2KHR;
        dev->disp.QueueBindSparse = vkl_QueueBindSparse;
        dev->disp.QueueWaitIdle = vkl_QueueWaitIdle;
        dev->disp.DeviceWaitIdle = vkl_DeviceWaitIdle;

        vkl_wsi_install_device(dev);
    }

    *pDevice = dev->handle;
    return VK_SUCCESS;
}

void vkl_device_install(struct vkl_icd_instance *ii)
{
    ii->disp.CreateDevice = vkl_CreateDevice;
    ii->disp.EnumerateDeviceExtensionProperties = vkl_EnumerateDeviceExtensionProperties;
    ii->disp.EnumerateDeviceLayerProperties = vkl_EnumerateDeviceLayerProperties;
}

/* ---- vkGetDeviceProcAddr ---- */

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *pName)
{
    struct vkl_device *dev;
    const struct vkl_command *cmd;

    if (!device || !pName)
        return NULL;
    dev = VKL_DEVICE(device);

    if (!strcmp(pName, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction)vkGetDeviceProcAddr;

    cmd = vkl_find_command(pName);
    if (!cmd)
        return dev->icd->gdpa(device, pName);   /* newer than the library */
    if (cmd->level != VKL_LEVEL_DEVICE)
        return NULL;

    /* the driver's function itself, or the library's where it overrides */
    return VKL_SLOT(&dev->disp, cmd);
}
