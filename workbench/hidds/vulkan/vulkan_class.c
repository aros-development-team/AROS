/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan base HIDD class — abstract stubs.
    Hardware drivers (RADV, lavapipe, etc.) subclass and override.

    All methods dispatch to VulkanDriverCallbacks on the preferred GPU.
    If no driver is registered, methods return error codes or do nothing.
*/

#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include <hidd/vulkan_hidd.h>

#include "vulkan_intern.h"

#undef HiddVulkanAttrBase
#define HiddVulkanAttrBase   (SD(cl)->vulkanAttrBase)

/* VK_ERROR_INITIALIZATION_FAILED */
#define VK_INIT_FAIL ((IPTR)(-3))

/* Helper: get preferred GPU or NULL */
#define GET_GPU(sd) VulkanGetPreferredGPU(sd)
/* Helper: get preferred display-capable GPU for presentation paths */
#define GET_DISPLAY_GPU(sd) VulkanGetPreferredDisplayGPU(sd)

/* ---- Root interface ---- */

OOP_Object *METHOD(HiddVulkan, Root, New)
{
    IPTR interfaceVers;

    D(bug("[Vulkan] %s()\n", __func__));

    interfaceVers = GetTagData(aHidd_Vulkan_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != VULKAN_INTERFACE_VERSION)
    {
        D(bug("[Vulkan] %s: interface version mismatch (got %ld, want %d)\n",
              __func__, (long)interfaceVers, VULKAN_INTERFACE_VERSION));
        return NULL;
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(HiddVulkan, Root, Get)
{
    ULONG idx;

    D(bug("[Vulkan] %s() attrID=%lx\n", __func__, (unsigned long)msg->attrID));

    if (IS_VULKAN_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Vulkan_InterfaceVersion:
                *msg->storage = VULKAN_INTERFACE_VERSION;
                return;
            case aoHidd_Vulkan_GPUCount:
                *msg->storage = (IPTR)SD(cl)->gpu_count;
                return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ==================================================================
 * Instance lifecycle
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, CreateInstance)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    D(bug("[Vulkan] %s(app='%s')\n", __func__,
          msg->applicationName ? msg->applicationName : "(null)"));
    if (gpu && gpu->callbacks.CreateInstance)
        return gpu->callbacks.CreateInstance(msg->applicationName,
                                             msg->applicationVersion,
                                             gpu->driver_data);
    return NULL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyInstance)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyInstance)
        gpu->callbacks.DestroyInstance(msg->instance, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumeratePhysicalDevices)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.EnumeratePhysicalDevices)
        return (IPTR)gpu->callbacks.EnumeratePhysicalDevices(
            msg->instance, msg->pPhysicalDeviceCount,
            msg->pPhysicalDevices, gpu->driver_data);
    if (msg->pPhysicalDeviceCount) *msg->pPhysicalDeviceCount = 0;
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Physical device queries
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceProperties)
        gpu->callbacks.GetPhysicalDeviceProperties(
            msg->physicalDevice, msg->pProperties, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties2)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceProperties2)
        gpu->callbacks.GetPhysicalDeviceProperties2(
            msg->physicalDevice, msg->pProperties, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceFeatures)
        gpu->callbacks.GetPhysicalDeviceFeatures(
            msg->physicalDevice, msg->pFeatures, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures2)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceFeatures2)
        gpu->callbacks.GetPhysicalDeviceFeatures2(
            msg->physicalDevice, msg->pFeatures, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceMemoryProperties)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceMemoryProperties)
        gpu->callbacks.GetPhysicalDeviceMemoryProperties(
            msg->physicalDevice, msg->pMemoryProperties, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceQueueFamilyProperties)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetPhysicalDeviceQueueFamilyProperties)
        gpu->callbacks.GetPhysicalDeviceQueueFamilyProperties(
            msg->physicalDevice, msg->pQueueFamilyPropertyCount,
            msg->pQueueFamilyProperties, gpu->driver_data);
    else if (msg->pQueueFamilyPropertyCount)
        *msg->pQueueFamilyPropertyCount = 0;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateDeviceExtensionProperties)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.EnumerateDeviceExtensionProperties)
        return (IPTR)gpu->callbacks.EnumerateDeviceExtensionProperties(
            msg->physicalDevice, msg->pLayerName,
            msg->pPropertyCount, msg->pProperties, gpu->driver_data);
    if (msg->pPropertyCount) *msg->pPropertyCount = 0;
    return 0;
}

/* ==================================================================
 * Device lifecycle
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDevice)
{
    struct vulkanstaticdata *sd = SD(cl);
    struct VulkanGPUEntry *gpu = GET_GPU(sd);
    APTR device;

    D(bug("[Vulkan] %s(physDev=%p)\n", __func__, msg->physicalDevice));

    if (!gpu || !gpu->callbacks.CreateDevice)
        return NULL;

    ObtainSemaphoreShared(&sd->registry_lock);
    if (gpu->locked) {
        ReleaseSemaphore(&sd->registry_lock);
        return NULL;
    }
    ReleaseSemaphore(&sd->registry_lock);

    device = gpu->callbacks.CreateDevice(msg->physicalDevice,
                                          msg->pCreateInfo,
                                          gpu->driver_data);
    if (device) {
        ObtainSemaphore(&sd->registry_lock);
        gpu->use_count++;
        ReleaseSemaphore(&sd->registry_lock);
    }
    return device;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDevice)
{
    struct vulkanstaticdata *sd = SD(cl);
    struct VulkanGPUEntry *gpu = GET_GPU(sd);

    if (gpu && gpu->callbacks.DestroyDevice) {
        gpu->callbacks.DestroyDevice(msg->device, gpu->driver_data);
        ObtainSemaphore(&sd->registry_lock);
        if (gpu->use_count > 0) gpu->use_count--;
        if (gpu->use_count == 0) gpu->locked = FALSE;
        ReleaseSemaphore(&sd->registry_lock);
    }
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetDeviceQueue)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetDeviceQueue)
        gpu->callbacks.GetDeviceQueue(msg->device, msg->queueFamilyIndex,
                                       msg->queueIndex, msg->pQueue,
                                       gpu->driver_data);
    else if (msg->pQueue)
        *msg->pQueue = NULL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, DeviceWaitIdle)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DeviceWaitIdle)
        return (IPTR)gpu->callbacks.DeviceWaitIdle(msg->device,
                                                    gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Display (optional)
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, DisplayResource)
{
    struct VulkanGPUEntry *gpu = GET_DISPLAY_GPU(SD(cl));
    if (gpu && gpu->callbacks.DisplayResource)
        gpu->callbacks.DisplayResource(msg->image, msg->srcx, msg->srcy,
                                        msg->rastPort, msg->dstx, msg->dsty,
                                        msg->width, msg->height,
                                        gpu->driver_data);
}

/* ==================================================================
 * Memory management
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateMemory)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.AllocateMemory)
        return (IPTR)gpu->callbacks.AllocateMemory(
            msg->device, msg->allocationSize, msg->memoryTypeIndex,
            msg->pMemory, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeMemory)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.FreeMemory)
        gpu->callbacks.FreeMemory(msg->device, msg->memory, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, MapMemory)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.MapMemory)
        return (IPTR)gpu->callbacks.MapMemory(
            msg->device, msg->memory, msg->offset, msg->size,
            msg->flags, msg->ppData, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, UnmapMemory)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.UnmapMemory)
        gpu->callbacks.UnmapMemory(msg->device, msg->memory, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetMemoryHostPointerPropertiesEXT)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetMemoryHostPointerPropertiesEXT)
        return (IPTR)gpu->callbacks.GetMemoryHostPointerPropertiesEXT(
            msg->device, msg->handleType, msg->pHostPointer,
            msg->pMemoryHostPointerProperties, gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Buffers
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateBuffer)
        return (IPTR)gpu->callbacks.CreateBuffer(
            msg->device, msg->size, msg->usage, msg->sharingMode,
            msg->pBuffer, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyBuffer)
        gpu->callbacks.DestroyBuffer(msg->device, msg->buffer,
                                      gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BindBufferMemory)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.BindBufferMemory)
        return (IPTR)gpu->callbacks.BindBufferMemory(
            msg->device, msg->buffer, msg->memory,
            msg->memoryOffset, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetBufferMemoryRequirements)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetBufferMemoryRequirements)
        gpu->callbacks.GetBufferMemoryRequirements(
            msg->device, msg->buffer, msg->pMemoryRequirements,
            gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetBufferDeviceAddress)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetBufferDeviceAddress)
        return (IPTR)gpu->callbacks.GetBufferDeviceAddress(
            msg->device, msg->pInfo, gpu->driver_data);
    return 0;
}

/* ==================================================================
 * Descriptor sets
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDescriptorSetLayout)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateDescriptorSetLayout)
        return (IPTR)gpu->callbacks.CreateDescriptorSetLayout(
            msg->device, msg->pCreateInfo, msg->pSetLayout,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorSetLayout)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyDescriptorSetLayout)
        gpu->callbacks.DestroyDescriptorSetLayout(
            msg->device, msg->descriptorSetLayout, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDescriptorPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateDescriptorPool)
        return (IPTR)gpu->callbacks.CreateDescriptorPool(
            msg->device, msg->pCreateInfo, msg->pDescriptorPool,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyDescriptorPool)
        gpu->callbacks.DestroyDescriptorPool(
            msg->device, msg->descriptorPool, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateDescriptorSets)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.AllocateDescriptorSets)
        return (IPTR)gpu->callbacks.AllocateDescriptorSets(
            msg->device, msg->pAllocateInfo, msg->pDescriptorSets,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, UpdateDescriptorSets)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.UpdateDescriptorSets)
        gpu->callbacks.UpdateDescriptorSets(
            msg->device, msg->descriptorWriteCount, msg->pDescriptorWrites,
            msg->descriptorCopyCount, msg->pDescriptorCopies,
            gpu->driver_data);
}

/* ==================================================================
 * Command pools and buffers
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateCommandPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateCommandPool)
        return (IPTR)gpu->callbacks.CreateCommandPool(
            msg->device, msg->queueFamilyIndex, msg->flags,
            msg->pCommandPool, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyCommandPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyCommandPool)
        gpu->callbacks.DestroyCommandPool(msg->device, msg->commandPool,
                                           gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetCommandPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.ResetCommandPool)
        return (IPTR)gpu->callbacks.ResetCommandPool(
            msg->device, msg->commandPool, msg->flags, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateCommandBuffers)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.AllocateCommandBuffers)
        return (IPTR)gpu->callbacks.AllocateCommandBuffers(
            msg->device, msg->commandPool, msg->level, msg->count,
            msg->pCommandBuffers, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeCommandBuffers)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.FreeCommandBuffers)
        gpu->callbacks.FreeCommandBuffers(
            msg->device, msg->commandPool, msg->count,
            msg->pCommandBuffers, gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BeginCommandBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.BeginCommandBuffer)
        return (IPTR)gpu->callbacks.BeginCommandBuffer(
            msg->commandBuffer, msg->flags, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EndCommandBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.EndCommandBuffer)
        return (IPTR)gpu->callbacks.EndCommandBuffer(
            msg->commandBuffer, gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Queue submission
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueSubmit)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.QueueSubmit)
        return (IPTR)gpu->callbacks.QueueSubmit(
            msg->queue, msg->submitCount, msg->pSubmits,
            msg->fence, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueWaitIdle)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.QueueWaitIdle)
        return (IPTR)gpu->callbacks.QueueWaitIdle(msg->queue,
                                                   gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Fences
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateFence)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateFence)
        return (IPTR)gpu->callbacks.CreateFence(
            msg->device, msg->flags, msg->pFence, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyFence)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyFence)
        gpu->callbacks.DestroyFence(msg->device, msg->fence,
                                     gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, WaitForFences)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.WaitForFences)
        return (IPTR)gpu->callbacks.WaitForFences(
            msg->device, msg->fenceCount, msg->pFences,
            msg->waitAll, msg->timeout, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetFences)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.ResetFences)
        return (IPTR)gpu->callbacks.ResetFences(
            msg->device, msg->fenceCount, msg->pFences, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetFenceStatus)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetFenceStatus)
        return (IPTR)gpu->callbacks.GetFenceStatus(
            msg->device, msg->fence, gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Semaphores
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateSemaphore)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateSemaphore)
        return (IPTR)gpu->callbacks.CreateSemaphore(
            msg->device, msg->pCreateInfo, msg->pSemaphore,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroySemaphore)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroySemaphore)
        gpu->callbacks.DestroySemaphore(msg->device, msg->semaphore,
                                         gpu->driver_data);
}

/* ==================================================================
 * Events
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateEvent)
        return (IPTR)gpu->callbacks.CreateEvent(
            msg->device, msg->pEvent, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyEvent)
        gpu->callbacks.DestroyEvent(msg->device, msg->event,
                                     gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, SetEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.SetEvent)
        return (IPTR)gpu->callbacks.SetEvent(
            msg->device, msg->event, gpu->driver_data);
    return VK_INIT_FAIL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.ResetEvent)
        return (IPTR)gpu->callbacks.ResetEvent(
            msg->device, msg->event, gpu->driver_data);
    return VK_INIT_FAIL;
}

/* ==================================================================
 * Shader / pipeline
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateShaderModule)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateShaderModule)
        return (IPTR)gpu->callbacks.CreateShaderModule(
            msg->device, msg->pCode, msg->codeSize,
            msg->pShaderModule, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyShaderModule)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyShaderModule)
        gpu->callbacks.DestroyShaderModule(msg->device, msg->shaderModule,
                                            gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreatePipelineLayout)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreatePipelineLayout)
        return (IPTR)gpu->callbacks.CreatePipelineLayout(
            msg->device, msg->setLayoutCount, msg->pSetLayouts,
            msg->pushConstantRangeCount, msg->pPushConstantRanges,
            msg->pPipelineLayout, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipelineLayout)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyPipelineLayout)
        gpu->callbacks.DestroyPipelineLayout(msg->device,
                                              msg->pipelineLayout,
                                              gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateComputePipelines)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateComputePipelines)
        return (IPTR)gpu->callbacks.CreateComputePipelines(
            msg->device, msg->createInfoCount, msg->pCreateInfos,
            msg->pPipelines, gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipeline)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyPipeline)
        gpu->callbacks.DestroyPipeline(msg->device, msg->pipeline,
                                        gpu->driver_data);
}

/* ==================================================================
 * Query pools
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateQueryPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CreateQueryPool)
        return (IPTR)gpu->callbacks.CreateQueryPool(
            msg->device, msg->pCreateInfo, msg->pQueryPool,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyQueryPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.DestroyQueryPool)
        gpu->callbacks.DestroyQueryPool(msg->device, msg->queryPool,
                                         gpu->driver_data);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetQueryPoolResults)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetQueryPoolResults)
        return (IPTR)gpu->callbacks.GetQueryPoolResults(
            msg->device, msg->queryPool, msg->firstQuery, msg->queryCount,
            msg->dataSize, msg->pData, msg->stride, msg->flags,
            gpu->driver_data);
    return VK_INIT_FAIL;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, ResetQueryPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.ResetQueryPool)
        gpu->callbacks.ResetQueryPool(msg->device, msg->queryPool,
                                       msg->firstQuery, msg->queryCount,
                                       gpu->driver_data);
}

/* ==================================================================
 * Command recording — compute
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindPipeline)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdBindPipeline)
        gpu->callbacks.CmdBindPipeline(msg->commandBuffer,
                                        msg->pipelineBindPoint, msg->pipeline,
                                        gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindDescriptorSets)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdBindDescriptorSets)
        gpu->callbacks.CmdBindDescriptorSets(
            msg->commandBuffer, msg->pipelineBindPoint, msg->layout,
            msg->firstSet, msg->descriptorSetCount, msg->pDescriptorSets,
            msg->dynamicOffsetCount, msg->pDynamicOffsets,
            gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPushConstants)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdPushConstants)
        gpu->callbacks.CmdPushConstants(
            msg->commandBuffer, msg->layout, msg->stageFlags,
            msg->offset, msg->size, msg->pValues, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdDispatch)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdDispatch)
        gpu->callbacks.CmdDispatch(msg->commandBuffer,
                                    msg->groupCountX, msg->groupCountY,
                                    msg->groupCountZ, gpu->driver_data);
}

/* ==================================================================
 * Command recording — transfer
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdCopyBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdCopyBuffer)
        gpu->callbacks.CmdCopyBuffer(msg->commandBuffer,
                                      msg->srcBuffer, msg->dstBuffer,
                                      msg->regionCount, msg->pRegions,
                                      gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdFillBuffer)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdFillBuffer)
        gpu->callbacks.CmdFillBuffer(msg->commandBuffer,
                                      msg->dstBuffer, msg->dstOffset,
                                      msg->size, msg->data,
                                      gpu->driver_data);
}

/* ==================================================================
 * Command recording — synchronization
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPipelineBarrier)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdPipelineBarrier)
        gpu->callbacks.CmdPipelineBarrier(
            msg->commandBuffer, msg->srcStageMask, msg->dstStageMask,
            msg->dependencyFlags,
            msg->memoryBarrierCount, msg->pMemoryBarriers,
            msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
            msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers,
            gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdSetEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdSetEvent)
        gpu->callbacks.CmdSetEvent(msg->commandBuffer, msg->event,
                                    msg->stageMask, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetEvent)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdResetEvent)
        gpu->callbacks.CmdResetEvent(msg->commandBuffer, msg->event,
                                      msg->stageMask, gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWaitEvents)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdWaitEvents)
        gpu->callbacks.CmdWaitEvents(
            msg->commandBuffer, msg->eventCount, msg->pEvents,
            msg->srcStageMask, msg->dstStageMask,
            msg->memoryBarrierCount, msg->pMemoryBarriers,
            msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
            msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers,
            gpu->driver_data);
}

/* ==================================================================
 * Command recording — queries
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWriteTimestamp)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdWriteTimestamp)
        gpu->callbacks.CmdWriteTimestamp(msg->commandBuffer,
                                          msg->pipelineStage,
                                          msg->queryPool, msg->query,
                                          gpu->driver_data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetQueryPool)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.CmdResetQueryPool)
        gpu->callbacks.CmdResetQueryPool(msg->commandBuffer,
                                          msg->queryPool,
                                          msg->firstQuery, msg->queryCount,
                                          gpu->driver_data);
}

/* ==================================================================
 * GPU registry management
 * ================================================================== */

ULONG METHOD(HiddVulkan, Hidd_Vulkan, EnumerateGPUs)
{
    struct vulkanstaticdata *sd = SD(cl);
    ULONG count = 0;
    ULONG i;

    ObtainSemaphoreShared(&sd->registry_lock);

    for (i = 0; i < VULKAN_MAX_GPUS && count < msg->max_count; i++)
    {
        if (sd->gpus[i].in_use)
        {
            if (msg->gpu_info)
            {
                struct VulkanGPUInfo *out = &msg->gpu_info[count];
                struct VulkanGPUEntry *e  = &sd->gpus[i];
                ULONG j;

                out->vendor_id = e->vendor_id;
                out->device_id = e->device_id;
                out->flags     = e->flags;
                out->priority  = e->priority;
                out->vram_size = e->vram_size;
                out->pci_bus   = e->pci_bus;
                out->pci_dev   = e->pci_dev;
                out->pci_func  = e->pci_func;

                for (j = 0; j < sizeof(out->name) - 1 && e->name[j]; j++)
                    out->name[j] = e->name[j];
                out->name[j] = '\0';

                for (j = 0; j < sizeof(out->class_id) - 1 && e->class_id[j]; j++)
                    out->class_id[j] = e->class_id[j];
                out->class_id[j] = '\0';
            }
            count++;
        }
    }

    ReleaseSemaphore(&sd->registry_lock);

    return count;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, SetGPUPriority)
{
    struct vulkanstaticdata *sd = SD(cl);
    ULONG target_idx = msg->gpu_index;
    ULONG count = 0;
    ULONG i;

    ObtainSemaphore(&sd->registry_lock);

    for (i = 0; i < VULKAN_MAX_GPUS; i++)
    {
        if (sd->gpus[i].in_use)
        {
            if (count == target_idx)
            {
                sd->gpus[i].priority = msg->priority;
                break;
            }
            count++;
        }
    }

    ReleaseSemaphore(&sd->registry_lock);
}

/* ==================================================================
 * ICD-style function lookup
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, GetInstanceProcAddr)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetInstanceProcAddr)
        return gpu->callbacks.GetInstanceProcAddr(
            msg->instance, msg->pName, gpu->driver_data);
    return NULL;
}

APTR METHOD(HiddVulkan, Hidd_Vulkan, GetDeviceProcAddr)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.GetDeviceProcAddr)
        return gpu->callbacks.GetDeviceProcAddr(
            msg->device, msg->pName, gpu->driver_data);
    if (gpu && gpu->callbacks.GetInstanceProcAddr)
        return gpu->callbacks.GetInstanceProcAddr(
            NULL, msg->pName, gpu->driver_data);
    return NULL;
}

/* ==================================================================
 * Instance-level queries
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateInstanceVersion)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.EnumerateInstanceVersion)
        return (IPTR)gpu->callbacks.EnumerateInstanceVersion(
            msg->pApiVersion, gpu->driver_data);
    /* No driver — report Vulkan 1.0 */
    if (msg->pApiVersion) *msg->pApiVersion = (1 << 22); /* VK_API_VERSION_1_0 */
    return 0;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateInstanceExtensionProperties)
{
    struct VulkanGPUEntry *gpu = GET_GPU(SD(cl));
    if (gpu && gpu->callbacks.EnumerateInstanceExtensionProperties)
        return (IPTR)gpu->callbacks.EnumerateInstanceExtensionProperties(
            msg->pLayerName, msg->pPropertyCount, msg->pProperties,
            gpu->driver_data);
    if (msg->pPropertyCount) *msg->pPropertyCount = 0;
    return 0;
}
