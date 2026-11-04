/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan base HIDD class. A driver subclasses it, overriding
    GetInstanceProcAddr with its ICD's entry point; every other method here
    is implemented through that, so the driver needs nothing else to serve
    the loader (vulkan_loader.c) and gallium-on-Vulkan (zink.hidd). Objects
    come from the display driver's CreateObject, or are the software
    driver's. Without an overriding GetInstanceProcAddr the methods fail,
    as for a device without Vulkan.

    Handles cross the interface as APTR; on the 64-bit targets every Vulkan
    handle fits one, arrays of handles included.
*/

#include <aros/debug.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <hidd/vulkan_hidd.h>

#include "vulkan_intern.h"

#undef HiddVulkanAttrBase
#define HiddVulkanAttrBase   (SD(cl)->vulkanAttrBase)

#define VK_INIT_FAIL ((IPTR)VK_ERROR_INITIALIZATION_FAILED)

/* a handle from / to the interface's APTR */
#define H(type, v)      ((type)(IPTR)(v))
#define HA(type, v)     ((type *)(v))
#define OUT(p, v)       (*(p) = (APTR)(IPTR)(v))

/*
 * The driver's entry point for a Vulkan function: through the subclass's
 * GetInstanceProcAddr, asked once for vkGetInstanceProcAddr itself, then
 * directly. Device commands resolve through the instance the object
 * created, as the specification allows.
 */
static PFN_vkVoidFunction vkproc(OOP_Class *cl, OOP_Object *o, const char *name)
{
    struct HiddVulkanData *data = OOP_INST_DATA(cl, o);

    if (!data->gipa)
    {
        struct pHidd_Vulkan_GetInstanceProcAddr msg;

        msg.mID = SD(cl)->mbase + moHidd_Vulkan_GetInstanceProcAddr;
        msg.instance = NULL;
        msg.pName = "vkGetInstanceProcAddr";
        data->gipa = (PFN_vkGetInstanceProcAddr)OOP_DoMethod(o, (OOP_Msg)&msg);
        if (!data->gipa)
            return NULL;
    }
    return ((PFN_vkGetInstanceProcAddr)data->gipa)(H(VkInstance, data->instance), name);
}

#define VKFN(name)  PFN_vk##name fn = (PFN_vk##name)vkproc(cl, o, "vk" #name)
/* a method returning the VkResult of its function */
#define VKCALL(name, ...) \
    do { VKFN(name); if (!fn) return VK_INIT_FAIL; return (IPTR)fn(__VA_ARGS__); } while (0)
/* a method returning nothing */
#define VKVOID(name, ...) \
    do { VKFN(name); if (fn) fn(__VA_ARGS__); } while (0)

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

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HiddVulkanData *data = OOP_INST_DATA(cl, o);

        data->gipa = NULL;
        data->instance = NULL;
    }
    return o;
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
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* ==================================================================
 * Instance lifecycle
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, CreateInstance)
{
    struct HiddVulkanData *data = OOP_INST_DATA(cl, o);
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = msg->applicationName,
        .applicationVersion = msg->applicationVersion,
        .pEngineName = "AROS",
        .apiVersion = VK_API_VERSION_1_2,
    };
    VkInstanceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
    };
    VkInstance instance = VK_NULL_HANDLE;
    VKFN(CreateInstance);

    D(bug("[Vulkan] %s(app='%s')\n", __func__,
          msg->applicationName ? msg->applicationName : "(null)"));
    if (!fn || fn(&info, NULL, &instance) != VK_SUCCESS)
        return NULL;
    /* the first instance is the one device commands resolve through */
    if (!data->instance)
        data->instance = (APTR)instance;
    return (APTR)instance;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyInstance)
{
    struct HiddVulkanData *data = OOP_INST_DATA(cl, o);

    VKVOID(DestroyInstance, H(VkInstance, msg->instance), NULL);
    if (data->instance == msg->instance)
        data->instance = NULL;
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumeratePhysicalDevices)
{
    VKCALL(EnumeratePhysicalDevices, H(VkInstance, msg->instance),
           (uint32_t *)msg->pPhysicalDeviceCount, HA(VkPhysicalDevice, msg->pPhysicalDevices));
}

/* ==================================================================
 * Physical device queries
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties)
{
    VKVOID(GetPhysicalDeviceProperties, H(VkPhysicalDevice, msg->physicalDevice), msg->pProperties);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties2)
{
    VKVOID(GetPhysicalDeviceProperties2, H(VkPhysicalDevice, msg->physicalDevice), msg->pProperties);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures)
{
    VKVOID(GetPhysicalDeviceFeatures, H(VkPhysicalDevice, msg->physicalDevice), msg->pFeatures);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures2)
{
    VKVOID(GetPhysicalDeviceFeatures2, H(VkPhysicalDevice, msg->physicalDevice), msg->pFeatures);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceMemoryProperties)
{
    VKVOID(GetPhysicalDeviceMemoryProperties, H(VkPhysicalDevice, msg->physicalDevice),
           msg->pMemoryProperties);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceQueueFamilyProperties)
{
    VKVOID(GetPhysicalDeviceQueueFamilyProperties, H(VkPhysicalDevice, msg->physicalDevice),
           (uint32_t *)msg->pQueueFamilyPropertyCount, msg->pQueueFamilyProperties);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateDeviceExtensionProperties)
{
    VKCALL(EnumerateDeviceExtensionProperties, H(VkPhysicalDevice, msg->physicalDevice),
           msg->pLayerName, (uint32_t *)msg->pPropertyCount, msg->pProperties);
}

/* ==================================================================
 * Device lifecycle
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDevice)
{
    VkDevice device = VK_NULL_HANDLE;
    VKFN(CreateDevice);

    D(bug("[Vulkan] %s(physDev=%p)\n", __func__, msg->physicalDevice));
    if (!fn || fn(H(VkPhysicalDevice, msg->physicalDevice), msg->pCreateInfo, NULL, &device) != VK_SUCCESS)
        return NULL;
    return (APTR)device;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDevice)
{
    VKVOID(DestroyDevice, H(VkDevice, msg->device), NULL);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetDeviceQueue)
{
    VkQueue queue = VK_NULL_HANDLE;

    VKVOID(GetDeviceQueue, H(VkDevice, msg->device), msg->queueFamilyIndex, msg->queueIndex, &queue);
    OUT(msg->pQueue, queue);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, DeviceWaitIdle)
{
    VKCALL(DeviceWaitIdle, H(VkDevice, msg->device));
}

/* ==================================================================
 * Display: a driver that can show an image overrides this
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, DisplayResource)
{
}

/* ==================================================================
 * Memory
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateMemory)
{
    VkMemoryAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = msg->allocationSize,
        .memoryTypeIndex = msg->memoryTypeIndex,
    };
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkResult res;
    VKFN(AllocateMemory);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &memory);
    OUT(msg->pMemory, memory);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeMemory)
{
    VKVOID(FreeMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, MapMemory)
{
    VKCALL(MapMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory),
           msg->offset, msg->size, msg->flags, (void **)msg->ppData);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, UnmapMemory)
{
    VKVOID(UnmapMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory));
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetMemoryHostPointerPropertiesEXT)
{
    VKCALL(GetMemoryHostPointerPropertiesEXT, H(VkDevice, msg->device), msg->handleType,
           msg->pHostPointer, msg->pMemoryHostPointerProperties);
}

/* ==================================================================
 * Buffers
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateBuffer)
{
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = msg->size,
        .usage = msg->usage,
        .sharingMode = msg->sharingMode,
    };
    VkBuffer buffer = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateBuffer);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &buffer);
    OUT(msg->pBuffer, buffer);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyBuffer)
{
    VKVOID(DestroyBuffer, H(VkDevice, msg->device), H(VkBuffer, msg->buffer), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BindBufferMemory)
{
    VKCALL(BindBufferMemory, H(VkDevice, msg->device), H(VkBuffer, msg->buffer),
           H(VkDeviceMemory, msg->memory), msg->memoryOffset);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetBufferMemoryRequirements)
{
    VKVOID(GetBufferMemoryRequirements, H(VkDevice, msg->device), H(VkBuffer, msg->buffer),
           msg->pMemoryRequirements);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetBufferDeviceAddress)
{
    VKFN(GetBufferDeviceAddress);

    if (!fn)
        return 0;
    return (IPTR)fn(H(VkDevice, msg->device), msg->pInfo);
}

/* ==================================================================
 * Descriptor sets
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDescriptorSetLayout)
{
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateDescriptorSetLayout);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), msg->pCreateInfo, NULL, &layout);
    OUT(msg->pSetLayout, layout);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorSetLayout)
{
    VKVOID(DestroyDescriptorSetLayout, H(VkDevice, msg->device),
           H(VkDescriptorSetLayout, msg->descriptorSetLayout), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDescriptorPool)
{
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateDescriptorPool);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), msg->pCreateInfo, NULL, &pool);
    OUT(msg->pDescriptorPool, pool);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorPool)
{
    VKVOID(DestroyDescriptorPool, H(VkDevice, msg->device), H(VkDescriptorPool, msg->descriptorPool), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateDescriptorSets)
{
    VKCALL(AllocateDescriptorSets, H(VkDevice, msg->device), msg->pAllocateInfo,
           HA(VkDescriptorSet, msg->pDescriptorSets));
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, UpdateDescriptorSets)
{
    VKVOID(UpdateDescriptorSets, H(VkDevice, msg->device), msg->descriptorWriteCount,
           msg->pDescriptorWrites, msg->descriptorCopyCount, msg->pDescriptorCopies);
}

/* ==================================================================
 * Command pools and buffers
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateCommandPool)
{
    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = msg->flags,
        .queueFamilyIndex = msg->queueFamilyIndex,
    };
    VkCommandPool pool = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateCommandPool);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &pool);
    OUT(msg->pCommandPool, pool);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyCommandPool)
{
    VKVOID(DestroyCommandPool, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetCommandPool)
{
    VKCALL(ResetCommandPool, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool), msg->flags);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateCommandBuffers)
{
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = H(VkCommandPool, msg->commandPool),
        .level = msg->level,
        .commandBufferCount = msg->count,
    };

    VKCALL(AllocateCommandBuffers, H(VkDevice, msg->device), &info, HA(VkCommandBuffer, msg->pCommandBuffers));
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeCommandBuffers)
{
    VKVOID(FreeCommandBuffers, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool),
           msg->count, HA(const VkCommandBuffer, msg->pCommandBuffers));
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BeginCommandBuffer)
{
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = msg->flags,
    };

    VKCALL(BeginCommandBuffer, H(VkCommandBuffer, msg->commandBuffer), &info);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EndCommandBuffer)
{
    VKCALL(EndCommandBuffer, H(VkCommandBuffer, msg->commandBuffer));
}

/* ==================================================================
 * Queue submission
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueSubmit)
{
    VKCALL(QueueSubmit, H(VkQueue, msg->queue), msg->submitCount, msg->pSubmits, H(VkFence, msg->fence));
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueWaitIdle)
{
    VKCALL(QueueWaitIdle, H(VkQueue, msg->queue));
}

/* ==================================================================
 * Fences
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateFence)
{
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = msg->flags,
    };
    VkFence fence = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateFence);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &fence);
    OUT(msg->pFence, fence);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyFence)
{
    VKVOID(DestroyFence, H(VkDevice, msg->device), H(VkFence, msg->fence), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, WaitForFences)
{
    VKCALL(WaitForFences, H(VkDevice, msg->device), msg->fenceCount, HA(const VkFence, msg->pFences),
           msg->waitAll, msg->timeout);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetFences)
{
    VKCALL(ResetFences, H(VkDevice, msg->device), msg->fenceCount, HA(const VkFence, msg->pFences));
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetFenceStatus)
{
    VKCALL(GetFenceStatus, H(VkDevice, msg->device), H(VkFence, msg->fence));
}

/* ==================================================================
 * Semaphores
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateSemaphore)
{
    VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore sem = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateSemaphore);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), msg->pCreateInfo ? msg->pCreateInfo : &info, NULL, &sem);
    OUT(msg->pSemaphore, sem);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroySemaphore)
{
    VKVOID(DestroySemaphore, H(VkDevice, msg->device), H(VkSemaphore, msg->semaphore), NULL);
}

/* ==================================================================
 * Events
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateEvent)
{
    VkEventCreateInfo info = { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
    VkEvent event = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateEvent);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &event);
    OUT(msg->pEvent, event);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyEvent)
{
    VKVOID(DestroyEvent, H(VkDevice, msg->device), H(VkEvent, msg->event), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, SetEvent)
{
    VKCALL(SetEvent, H(VkDevice, msg->device), H(VkEvent, msg->event));
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetEvent)
{
    VKCALL(ResetEvent, H(VkDevice, msg->device), H(VkEvent, msg->event));
}

/* ==================================================================
 * Shaders and pipelines
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateShaderModule)
{
    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = msg->codeSize,
        .pCode = (const uint32_t *)msg->pCode,
    };
    VkShaderModule module = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateShaderModule);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &module);
    OUT(msg->pShaderModule, module);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyShaderModule)
{
    VKVOID(DestroyShaderModule, H(VkDevice, msg->device), H(VkShaderModule, msg->shaderModule), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreatePipelineLayout)
{
    VkPipelineLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = msg->setLayoutCount,
        .pSetLayouts = HA(const VkDescriptorSetLayout, msg->pSetLayouts),
        .pushConstantRangeCount = msg->pushConstantRangeCount,
        .pPushConstantRanges = msg->pPushConstantRanges,
    };
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreatePipelineLayout);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), &info, NULL, &layout);
    OUT(msg->pPipelineLayout, layout);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipelineLayout)
{
    VKVOID(DestroyPipelineLayout, H(VkDevice, msg->device), H(VkPipelineLayout, msg->pipelineLayout), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateComputePipelines)
{
    VKCALL(CreateComputePipelines, H(VkDevice, msg->device), VK_NULL_HANDLE, msg->createInfoCount,
           msg->pCreateInfos, NULL, HA(VkPipeline, msg->pPipelines));
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipeline)
{
    VKVOID(DestroyPipeline, H(VkDevice, msg->device), H(VkPipeline, msg->pipeline), NULL);
}

/* ==================================================================
 * Query pools
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateQueryPool)
{
    VkQueryPool pool = VK_NULL_HANDLE;
    VkResult res;
    VKFN(CreateQueryPool);

    if (!fn)
        return VK_INIT_FAIL;
    res = fn(H(VkDevice, msg->device), msg->pCreateInfo, NULL, &pool);
    OUT(msg->pQueryPool, pool);
    return (IPTR)res;
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyQueryPool)
{
    VKVOID(DestroyQueryPool, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool), NULL);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetQueryPoolResults)
{
    VKCALL(GetQueryPoolResults, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount, msg->dataSize, msg->pData, msg->stride, msg->flags);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, ResetQueryPool)
{
    VKVOID(ResetQueryPool, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount);
}

/* ==================================================================
 * Command recording
 * ================================================================== */

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindPipeline)
{
    VKVOID(CmdBindPipeline, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineBindPoint,
           H(VkPipeline, msg->pipeline));
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindDescriptorSets)
{
    VKVOID(CmdBindDescriptorSets, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineBindPoint,
           H(VkPipelineLayout, msg->layout), msg->firstSet, msg->descriptorSetCount,
           HA(const VkDescriptorSet, msg->pDescriptorSets), msg->dynamicOffsetCount,
           (const uint32_t *)msg->pDynamicOffsets);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPushConstants)
{
    VKVOID(CmdPushConstants, H(VkCommandBuffer, msg->commandBuffer), H(VkPipelineLayout, msg->layout),
           msg->stageFlags, msg->offset, msg->size, msg->pValues);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdDispatch)
{
    VKVOID(CmdDispatch, H(VkCommandBuffer, msg->commandBuffer), msg->groupCountX, msg->groupCountY,
           msg->groupCountZ);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdCopyBuffer)
{
    VKVOID(CmdCopyBuffer, H(VkCommandBuffer, msg->commandBuffer), H(VkBuffer, msg->srcBuffer),
           H(VkBuffer, msg->dstBuffer), msg->regionCount, msg->pRegions);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdFillBuffer)
{
    VKVOID(CmdFillBuffer, H(VkCommandBuffer, msg->commandBuffer), H(VkBuffer, msg->dstBuffer),
           msg->dstOffset, msg->size, msg->data);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPipelineBarrier)
{
    VKVOID(CmdPipelineBarrier, H(VkCommandBuffer, msg->commandBuffer), msg->srcStageMask, msg->dstStageMask,
           msg->dependencyFlags, msg->memoryBarrierCount, msg->pMemoryBarriers,
           msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
           msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdSetEvent)
{
    VKVOID(CmdSetEvent, H(VkCommandBuffer, msg->commandBuffer), H(VkEvent, msg->event), msg->stageMask);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetEvent)
{
    VKVOID(CmdResetEvent, H(VkCommandBuffer, msg->commandBuffer), H(VkEvent, msg->event), msg->stageMask);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWaitEvents)
{
    VKVOID(CmdWaitEvents, H(VkCommandBuffer, msg->commandBuffer), msg->eventCount,
           HA(const VkEvent, msg->pEvents), msg->srcStageMask, msg->dstStageMask,
           msg->memoryBarrierCount, msg->pMemoryBarriers,
           msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
           msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWriteTimestamp)
{
    VKVOID(CmdWriteTimestamp, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineStage,
           H(VkQueryPool, msg->queryPool), msg->query);
}

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetQueryPool)
{
    VKVOID(CmdResetQueryPool, H(VkCommandBuffer, msg->commandBuffer), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount);
}

/* ==================================================================
 * ICD-style function lookup: GetInstanceProcAddr is the driver's
 * ================================================================== */

APTR METHOD(HiddVulkan, Hidd_Vulkan, GetInstanceProcAddr)
{
    return NULL;
}

APTR METHOD(HiddVulkan, Hidd_Vulkan, GetDeviceProcAddr)
{
    VKFN(GetDeviceProcAddr);

    if (!fn)
        return NULL;
    return (APTR)fn(H(VkDevice, msg->device), msg->pName);
}

/* ==================================================================
 * Instance-level queries
 * ================================================================== */

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateInstanceVersion)
{
    VKFN(EnumerateInstanceVersion);

    /* absent before Vulkan 1.1 */
    if (!fn)
    {
        *msg->pApiVersion = VK_API_VERSION_1_0;
        return (IPTR)VK_SUCCESS;
    }
    return (IPTR)fn((uint32_t *)msg->pApiVersion);
}

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateInstanceExtensionProperties)
{
    VKCALL(EnumerateInstanceExtensionProperties, msg->pLayerName, (uint32_t *)msg->pPropertyCount,
           msg->pProperties);
}
