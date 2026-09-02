/*
    vulkan_loader.c — AROS Vulkan Loader (libvulkan.a)

    Provides standard Vulkan entry points (vk*) that dispatch through
    vulkan.hidd via OOP method calls.

    This is the AROS equivalent of the Khronos Vulkan Loader (libvulkan.so
    on Linux).  Any AROS application that uses Vulkan should link -lvulkan.
    The loader opens vulkan.hidd at first use and routes all calls through
    VulkanDriverCallbacks to whichever driver is registered (venus.hidd,
    llvmpipe.hidd, etc.).

    GPU path for Venus (virgl/virtio-gpu):
        vk*() → OOP_DoMethod(vk_obj, ...)
              → vulkan.hidd → VulkanDriverCallbacks
              → venus.hidd → vtest socket
              → virglrenderer host → real GPU

    Copyright 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/oop.h>
#include <proto/exec.h>
#include <oop/oop.h>
#include <hidd/vulkan_hidd.h>
#include <stdint.h>
#include <string.h>

/* Keep this loader C-only and untyped at the public entry points.  The
 * Vulkan-Hpp runtime still needs the Vulkan 1.1 memory-properties wrapper,
 * so mirror only the structs needed to implement that entry point here. */
#define VK_LOADER_MAX_MEMORY_TYPES 32U
#define VK_LOADER_MAX_MEMORY_HEAPS 16U
#define VK_LOADER_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT 1000237000

typedef uint32_t VkLoaderStructureType;
typedef uint32_t VkLoaderMemoryPropertyFlags;
typedef uint32_t VkLoaderMemoryHeapFlags;
typedef uint64_t VkLoaderDeviceSize;

struct VkLoaderMemoryHeap {
    VkLoaderDeviceSize        size;
    VkLoaderMemoryHeapFlags   flags;
};

struct VkLoaderMemoryType {
    VkLoaderMemoryPropertyFlags propertyFlags;
    uint32_t                    heapIndex;
};

struct VkLoaderPhysicalDeviceMemoryProperties {
    uint32_t                    memoryTypeCount;
    struct VkLoaderMemoryType   memoryTypes[VK_LOADER_MAX_MEMORY_TYPES];
    uint32_t                    memoryHeapCount;
    struct VkLoaderMemoryHeap   memoryHeaps[VK_LOADER_MAX_MEMORY_HEAPS];
};

struct VkLoaderPhysicalDeviceMemoryProperties2 {
    VkLoaderStructureType                         sType;
    void                                         *pNext;
    struct VkLoaderPhysicalDeviceMemoryProperties memoryProperties;
};

struct VkLoaderBaseOutStructure {
    VkLoaderStructureType            sType;
    struct VkLoaderBaseOutStructure *pNext;
};

struct VkLoaderPhysicalDeviceMemoryBudgetPropertiesEXT {
    VkLoaderStructureType   sType;
    void                   *pNext;
    VkLoaderDeviceSize      heapBudget[VK_LOADER_MAX_MEMORY_HEAPS];
    VkLoaderDeviceSize      heapUsage[VK_LOADER_MAX_MEMORY_HEAPS];
};

/* ---- Global loader state ------------------------------------------ */

static OOP_Object   *vk_obj       = NULL;   /* HiddVulkan instance  */
static OOP_MethodID  vk_mbase     = 0;      /* IID_Hidd_Vulkan base */
static BOOL          vk_init_done = FALSE;

/* ---- Loader-owned command trampolines ---------------------------- */

void vkCmdBindPipeline(void *commandBuffer, uint32_t pipelineBindPoint,
                       void *pipeline);
void vkCmdBindDescriptorSets(void *commandBuffer, uint32_t pipelineBindPoint,
                             void *layout, uint32_t firstSet,
                             uint32_t descriptorSetCount,
                             const void *const *pDescriptorSets,
                             uint32_t dynamicOffsetCount,
                             const uint32_t *pDynamicOffsets);
void vkCmdPushConstants(void *commandBuffer, void *layout,
                        uint32_t stageFlags, uint32_t offset,
                        uint32_t size, const void *pValues);
void vkCmdDispatch(void *commandBuffer, uint32_t groupCountX,
                   uint32_t groupCountY, uint32_t groupCountZ);
void vkCmdCopyBuffer(void *commandBuffer, void *srcBuffer, void *dstBuffer,
                     uint32_t regionCount, const void *pRegions);
void vkCmdFillBuffer(void *commandBuffer, void *dstBuffer,
                     uint64_t dstOffset, uint64_t size, uint32_t data);
void vkCmdPipelineBarrier(void *commandBuffer, uint32_t srcStageMask,
                          uint32_t dstStageMask, uint32_t dependencyFlags,
                          uint32_t memoryBarrierCount,
                          const void *pMemoryBarriers,
                          uint32_t bufferMemoryBarrierCount,
                          const void *pBufferMemoryBarriers,
                          uint32_t imageMemoryBarrierCount,
                          const void *pImageMemoryBarriers);
void vkCmdSetEvent(void *commandBuffer, void *event, uint32_t stageMask);
void vkCmdResetEvent(void *commandBuffer, void *event, uint32_t stageMask);
void vkCmdWaitEvents(void *commandBuffer, uint32_t eventCount,
                     const void *const *pEvents, uint32_t srcStageMask,
                     uint32_t dstStageMask, uint32_t memoryBarrierCount,
                     const void *pMemoryBarriers,
                     uint32_t bufferMemoryBarrierCount,
                     const void *pBufferMemoryBarriers,
                     uint32_t imageMemoryBarrierCount,
                     const void *pImageMemoryBarriers);
void vkCmdWriteTimestamp(void *commandBuffer, uint32_t pipelineStage,
                         void *queryPool, uint32_t query);
void vkCmdResetQueryPool(void *commandBuffer, void *queryPool,
                         uint32_t firstQuery, uint32_t queryCount);
int32_t vkEnumerateInstanceVersion(uint32_t *pApiVersion);
int32_t vkEnumerateInstanceExtensionProperties(const char *pLayerName,
                                               uint32_t *pPropertyCount,
                                               void *pProperties);
int32_t vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
                                           void *pProperties);
int32_t vkCreateInstance(const void *pCreateInfo, const void *pAllocator,
                         void **pInstance);
void vkDestroyInstance(void *instance, const void *pAllocator);
int32_t vkEnumeratePhysicalDevices(void *instance,
                                   uint32_t *pPhysicalDeviceCount,
                                   void **pPhysicalDevices);
void vkGetPhysicalDeviceProperties(void *physicalDevice, void *pProperties);
void vkGetPhysicalDeviceProperties2(void *physicalDevice, void *pProperties);
void vkGetPhysicalDeviceFeatures(void *physicalDevice, void *pFeatures);
void vkGetPhysicalDeviceFeatures2(void *physicalDevice, void *pFeatures);
void vkGetPhysicalDeviceMemoryProperties(void *physicalDevice,
                                         void *pMemoryProperties);
void vkGetPhysicalDeviceMemoryProperties2(void *physicalDevice,
                                          void *pMemoryProperties);
void vkGetPhysicalDeviceMemoryProperties2KHR(void *physicalDevice,
                                             void *pMemoryProperties);
void vkGetPhysicalDeviceQueueFamilyProperties(void *physicalDevice,
                                              uint32_t *pQueueFamilyPropertyCount,
                                              void *pQueueFamilyProperties);
int32_t vkEnumerateDeviceExtensionProperties(void *physicalDevice,
                                             const char *pLayerName,
                                             uint32_t *pPropertyCount,
                                             void *pProperties);
int32_t vkCreateDevice(void *physicalDevice, const void *pCreateInfo,
                       const void *pAllocator, void **pDevice);
void vkDestroyDevice(void *device, const void *pAllocator);
void vkGetDeviceQueue(void *device, uint32_t queueFamilyIndex,
                      uint32_t queueIndex, void **pQueue);
int32_t vkDeviceWaitIdle(void *device);
void *vkGetDeviceProcAddr(void *device, const char *pName);
void *vkGetInstanceProcAddr(void *instance, const char *pName);

static void *vk_loader_local_proc(const char *pName)
{
    if (!pName)
        return NULL;

#define VK_LOADER_ENTRY(name) \
    if (strcmp(pName, #name) == 0) return (void *)name

    VK_LOADER_ENTRY(vkGetInstanceProcAddr);
    VK_LOADER_ENTRY(vkGetDeviceProcAddr);
    VK_LOADER_ENTRY(vkEnumerateInstanceVersion);
    VK_LOADER_ENTRY(vkEnumerateInstanceExtensionProperties);
    VK_LOADER_ENTRY(vkEnumerateInstanceLayerProperties);
    VK_LOADER_ENTRY(vkCreateInstance);
    VK_LOADER_ENTRY(vkDestroyInstance);
    VK_LOADER_ENTRY(vkEnumeratePhysicalDevices);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceProperties);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceProperties2);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceFeatures);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceFeatures2);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceMemoryProperties);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceMemoryProperties2);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceMemoryProperties2KHR);
    VK_LOADER_ENTRY(vkGetPhysicalDeviceQueueFamilyProperties);
    VK_LOADER_ENTRY(vkEnumerateDeviceExtensionProperties);
    VK_LOADER_ENTRY(vkCreateDevice);
    VK_LOADER_ENTRY(vkDestroyDevice);
    VK_LOADER_ENTRY(vkGetDeviceQueue);
    VK_LOADER_ENTRY(vkDeviceWaitIdle);

#undef VK_LOADER_ENTRY

    return NULL;
}

/* ---- One-time initialisation -------------------------------------- */

static void vk_loader_init(void)
{
    OOP_AttrBase attrBase;

    if (vk_init_done)
        return;
    vk_init_done = TRUE;

    attrBase = OOP_ObtainAttrBase(IID_Hidd_Vulkan);
    if (!attrBase)
        return;

    struct TagItem tags[] = {
        { attrBase + aoHidd_Vulkan_InterfaceVersion, VULKAN_INTERFACE_VERSION },
        { TAG_DONE, 0 }
    };
    vk_obj = OOP_NewObject(NULL, CLID_Hidd_Vulkan, tags);
    OOP_ReleaseAttrBase(IID_Hidd_Vulkan);
    if (!vk_obj)
        return;

    /* Cache the method ID base — used for all subsequent dispatch */
    vk_mbase = OOP_GetMethodID(IID_Hidd_Vulkan, 0);
}

/* ---- Root Vulkan entry points ------------------------------------- */

/*
 * vkGetInstanceProcAddr — THE root Vulkan entry point.
 * ggml-vulkan (and Vulkan-Hpp DispatchLoaderDynamic) bootstraps all
 * other function pointers through this call.
 */
void *vkGetInstanceProcAddr(void *instance, const char *pName)
{
    void *proc = vk_loader_local_proc(pName);
    if (proc)
        return proc;

    vk_loader_init();
    if (!vk_obj)
        return NULL;

    struct pHidd_Vulkan_GetInstanceProcAddr msg = {
        .mID      = vk_mbase + moHidd_Vulkan_GetInstanceProcAddr,
        .instance = instance,
        .pName    = pName,
    };
    return (void *)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void *vkGetDeviceProcAddr(void *device, const char *pName)
{
    void *proc = vk_loader_local_proc(pName);
    if (proc)
        return proc;

    vk_loader_init();
    if (!vk_obj)
        return NULL;

    struct pHidd_Vulkan_GetDeviceProcAddr msg = {
        .mID    = vk_mbase + moHidd_Vulkan_GetDeviceProcAddr,
        .device = device,
        .pName  = pName,
    };
    return (void *)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

/* ---- Instance/device bootstrap entry points ---------------------- */

int32_t vkEnumerateInstanceVersion(uint32_t *pApiVersion)
{
    ULONG apiVersion;
    int32_t result;

    vk_loader_init();
    if (!vk_obj) {
        if (pApiVersion)
            *pApiVersion = (1u << 22); /* VK_API_VERSION_1_0 */
        return 0;
    }

    apiVersion = pApiVersion ? *pApiVersion : 0;
    struct pHidd_Vulkan_EnumerateInstanceVersion msg = {
        .mID         = vk_mbase + moHidd_Vulkan_EnumerateInstanceVersion,
        .pApiVersion = &apiVersion,
    };
    result = (int32_t)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pApiVersion)
        *pApiVersion = (uint32_t)apiVersion;
    return result;
}

int32_t vkEnumerateInstanceExtensionProperties(const char *pLayerName,
                                               uint32_t *pPropertyCount,
                                               void *pProperties)
{
    ULONG propertyCount;
    int32_t result;

    vk_loader_init();
    if (!vk_obj) {
        if (pPropertyCount)
            *pPropertyCount = 0;
        return 0;
    }

    propertyCount = pPropertyCount ? *pPropertyCount : 0;
    struct pHidd_Vulkan_EnumerateInstanceExtensionProperties msg = {
        .mID            = vk_mbase + moHidd_Vulkan_EnumerateInstanceExtensionProperties,
        .pLayerName     = pLayerName,
        .pPropertyCount = &propertyCount,
        .pProperties    = pProperties,
    };
    result = (int32_t)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pPropertyCount)
        *pPropertyCount = (uint32_t)propertyCount;
    return result;
}

int32_t vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
                                           void *pProperties)
{
    (void)pProperties;
    if (pPropertyCount)
        *pPropertyCount = 0;
    return 0;
}

int32_t vkCreateInstance(const void *pCreateInfo, const void *pAllocator,
                         void **pInstance)
{
    void *instance;

    (void)pCreateInfo;
    (void)pAllocator;

    vk_loader_init();
    if (!vk_obj)
        return -3; /* VK_ERROR_INITIALIZATION_FAILED */

    struct pHidd_Vulkan_CreateInstance msg = {
        .mID                = vk_mbase + moHidd_Vulkan_CreateInstance,
        .applicationName    = "AROS Vulkan Loader",
        .applicationVersion = 1,
    };
    instance = (void *)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pInstance)
        *pInstance = instance;
    return instance ? 0 : -3;
}

void vkDestroyInstance(void *instance, const void *pAllocator)
{
    (void)pAllocator;

    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_DestroyInstance msg = {
        .mID      = vk_mbase + moHidd_Vulkan_DestroyInstance,
        .instance = instance,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

int32_t vkEnumeratePhysicalDevices(void *instance,
                                   uint32_t *pPhysicalDeviceCount,
                                   void **pPhysicalDevices)
{
    ULONG physicalDeviceCount;
    int32_t result;

    vk_loader_init();
    if (!vk_obj) {
        if (pPhysicalDeviceCount)
            *pPhysicalDeviceCount = 0;
        return -3;
    }

    physicalDeviceCount = pPhysicalDeviceCount ? *pPhysicalDeviceCount : 0;
    struct pHidd_Vulkan_EnumeratePhysicalDevices msg = {
        .mID                  = vk_mbase + moHidd_Vulkan_EnumeratePhysicalDevices,
        .instance             = instance,
        .pPhysicalDeviceCount = &physicalDeviceCount,
        .pPhysicalDevices     = (APTR *)pPhysicalDevices,
    };
    result = (int32_t)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pPhysicalDeviceCount)
        *pPhysicalDeviceCount = (uint32_t)physicalDeviceCount;
    return result;
}

void vkGetPhysicalDeviceProperties(void *physicalDevice, void *pProperties)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_GetPhysicalDeviceProperties msg = {
        .mID            = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceProperties,
        .physicalDevice = physicalDevice,
        .pProperties    = pProperties,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetPhysicalDeviceProperties2(void *physicalDevice, void *pProperties)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_GetPhysicalDeviceProperties2 msg = {
        .mID            = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceProperties2,
        .physicalDevice = physicalDevice,
        .pProperties    = pProperties,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetPhysicalDeviceFeatures(void *physicalDevice, void *pFeatures)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_GetPhysicalDeviceFeatures msg = {
        .mID            = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceFeatures,
        .physicalDevice = physicalDevice,
        .pFeatures      = pFeatures,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetPhysicalDeviceFeatures2(void *physicalDevice, void *pFeatures)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_GetPhysicalDeviceFeatures2 msg = {
        .mID            = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceFeatures2,
        .physicalDevice = physicalDevice,
        .pFeatures      = pFeatures,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetPhysicalDeviceMemoryProperties(void *physicalDevice,
                                         void *pMemoryProperties)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_GetPhysicalDeviceMemoryProperties msg = {
        .mID               = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceMemoryProperties,
        .physicalDevice    = physicalDevice,
        .pMemoryProperties = pMemoryProperties,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetPhysicalDeviceMemoryProperties2(void *physicalDevice,
                                          void *pMemoryProperties)
{
    struct VkLoaderPhysicalDeviceMemoryProperties2 *props2;
    struct VkLoaderBaseOutStructure *next;

    if (!pMemoryProperties)
        return;

    props2 = (struct VkLoaderPhysicalDeviceMemoryProperties2 *)pMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &props2->memoryProperties);

    next = (struct VkLoaderBaseOutStructure *)props2->pNext;
    while (next) {
        if (next->sType == VK_LOADER_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT) {
            struct VkLoaderPhysicalDeviceMemoryBudgetPropertiesEXT *budget =
                (struct VkLoaderPhysicalDeviceMemoryBudgetPropertiesEXT *)next;
            uint32_t i;

            for (i = 0; i < VK_LOADER_MAX_MEMORY_HEAPS; ++i) {
                VkLoaderDeviceSize heap_size = 0;
                if (i < props2->memoryProperties.memoryHeapCount)
                    heap_size = props2->memoryProperties.memoryHeaps[i].size;
                budget->heapBudget[i] = heap_size;
                budget->heapUsage[i] = 0;
            }
        }
        next = next->pNext;
    }
}

void vkGetPhysicalDeviceMemoryProperties2KHR(void *physicalDevice,
                                             void *pMemoryProperties)
{
    vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
}

void vkGetPhysicalDeviceQueueFamilyProperties(void *physicalDevice,
                                              uint32_t *pQueueFamilyPropertyCount,
                                              void *pQueueFamilyProperties)
{
    ULONG queueFamilyPropertyCount;

    vk_loader_init();
    if (!vk_obj) {
        if (pQueueFamilyPropertyCount)
            *pQueueFamilyPropertyCount = 0;
        return;
    }

    queueFamilyPropertyCount = pQueueFamilyPropertyCount ? *pQueueFamilyPropertyCount : 0;
    struct pHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties msg = {
        .mID                       = vk_mbase + moHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties,
        .physicalDevice            = physicalDevice,
        .pQueueFamilyPropertyCount = &queueFamilyPropertyCount,
        .pQueueFamilyProperties    = pQueueFamilyProperties,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pQueueFamilyPropertyCount)
        *pQueueFamilyPropertyCount = (uint32_t)queueFamilyPropertyCount;
}

int32_t vkEnumerateDeviceExtensionProperties(void *physicalDevice,
                                             const char *pLayerName,
                                             uint32_t *pPropertyCount,
                                             void *pProperties)
{
    ULONG propertyCount;
    int32_t result;

    vk_loader_init();
    if (!vk_obj) {
        if (pPropertyCount)
            *pPropertyCount = 0;
        return -3;
    }

    propertyCount = pPropertyCount ? *pPropertyCount : 0;
    struct pHidd_Vulkan_EnumerateDeviceExtensionProperties msg = {
        .mID            = vk_mbase + moHidd_Vulkan_EnumerateDeviceExtensionProperties,
        .physicalDevice = physicalDevice,
        .pLayerName     = pLayerName,
        .pPropertyCount = &propertyCount,
        .pProperties    = pProperties,
    };
    result = (int32_t)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pPropertyCount)
        *pPropertyCount = (uint32_t)propertyCount;
    return result;
}

int32_t vkCreateDevice(void *physicalDevice, const void *pCreateInfo,
                       const void *pAllocator, void **pDevice)
{
    void *device;

    (void)pAllocator;

    vk_loader_init();
    if (!vk_obj)
        return -3;

    struct pHidd_Vulkan_CreateDevice msg = {
        .mID            = vk_mbase + moHidd_Vulkan_CreateDevice,
        .physicalDevice = physicalDevice,
        .pCreateInfo    = (APTR)pCreateInfo,
    };
    device = (void *)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
    if (pDevice)
        *pDevice = device;
    return device ? 0 : -3;
}

void vkDestroyDevice(void *device, const void *pAllocator)
{
    (void)pAllocator;

    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_DestroyDevice msg = {
        .mID    = vk_mbase + moHidd_Vulkan_DestroyDevice,
        .device = device,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkGetDeviceQueue(void *device, uint32_t queueFamilyIndex,
                      uint32_t queueIndex, void **pQueue)
{
    vk_loader_init();
    if (!vk_obj) {
        if (pQueue)
            *pQueue = NULL;
        return;
    }

    struct pHidd_Vulkan_GetDeviceQueue msg = {
        .mID              = vk_mbase + moHidd_Vulkan_GetDeviceQueue,
        .device           = device,
        .queueFamilyIndex = queueFamilyIndex,
        .queueIndex       = queueIndex,
        .pQueue           = (APTR *)pQueue,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

int32_t vkDeviceWaitIdle(void *device)
{
    vk_loader_init();
    if (!vk_obj)
        return -3;

    struct pHidd_Vulkan_DeviceWaitIdle msg = {
        .mID    = vk_mbase + moHidd_Vulkan_DeviceWaitIdle,
        .device = device,
    };
    return (int32_t)OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

/* ---- Direct entry points referenced by ggml-vulkan.cpp ----------- */

void vkCmdBindPipeline(void *commandBuffer, uint32_t pipelineBindPoint,
                       void *pipeline)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdBindPipeline msg = {
        .mID               = vk_mbase + moHidd_Vulkan_CmdBindPipeline,
        .commandBuffer     = commandBuffer,
        .pipelineBindPoint = pipelineBindPoint,
        .pipeline          = pipeline,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdBindDescriptorSets(void *commandBuffer, uint32_t pipelineBindPoint,
                             void *layout, uint32_t firstSet,
                             uint32_t descriptorSetCount,
                             const void *const *pDescriptorSets,
                             uint32_t dynamicOffsetCount,
                             const uint32_t *pDynamicOffsets)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdBindDescriptorSets msg = {
        .mID                = vk_mbase + moHidd_Vulkan_CmdBindDescriptorSets,
        .commandBuffer      = commandBuffer,
        .pipelineBindPoint  = pipelineBindPoint,
        .layout             = layout,
        .firstSet           = firstSet,
        .descriptorSetCount = descriptorSetCount,
        .pDescriptorSets    = (const APTR *)pDescriptorSets,
        .dynamicOffsetCount = dynamicOffsetCount,
        .pDynamicOffsets    = (const ULONG *)pDynamicOffsets,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdPushConstants(void *commandBuffer, void *layout,
                        uint32_t stageFlags, uint32_t offset,
                        uint32_t size, const void *pValues)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdPushConstants msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdPushConstants,
        .commandBuffer = commandBuffer,
        .layout        = layout,
        .stageFlags    = stageFlags,
        .offset        = offset,
        .size          = size,
        .pValues       = (APTR)pValues,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdDispatch(void *commandBuffer, uint32_t groupCountX,
                   uint32_t groupCountY, uint32_t groupCountZ)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdDispatch msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdDispatch,
        .commandBuffer = commandBuffer,
        .groupCountX   = groupCountX,
        .groupCountY   = groupCountY,
        .groupCountZ   = groupCountZ,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

/*
 * vkCmdCopyBuffer — called directly in ggml_vk_buffer_copy_async().
 */
void vkCmdCopyBuffer(void *commandBuffer, void *srcBuffer, void *dstBuffer,
                     uint32_t regionCount, const void *pRegions)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdCopyBuffer msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdCopyBuffer,
        .commandBuffer = commandBuffer,
        .srcBuffer     = srcBuffer,
        .dstBuffer     = dstBuffer,
        .regionCount   = regionCount,
        .pRegions      = (APTR)pRegions,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdFillBuffer(void *commandBuffer, void *dstBuffer,
                     uint64_t dstOffset, uint64_t size, uint32_t data)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdFillBuffer msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdFillBuffer,
        .commandBuffer = commandBuffer,
        .dstBuffer     = dstBuffer,
        .dstOffset     = dstOffset,
        .size          = size,
        .data          = data,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdPipelineBarrier(void *commandBuffer, uint32_t srcStageMask,
                          uint32_t dstStageMask, uint32_t dependencyFlags,
                          uint32_t memoryBarrierCount,
                          const void *pMemoryBarriers,
                          uint32_t bufferMemoryBarrierCount,
                          const void *pBufferMemoryBarriers,
                          uint32_t imageMemoryBarrierCount,
                          const void *pImageMemoryBarriers)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdPipelineBarrier msg = {
        .mID                      = vk_mbase + moHidd_Vulkan_CmdPipelineBarrier,
        .commandBuffer            = commandBuffer,
        .srcStageMask             = srcStageMask,
        .dstStageMask             = dstStageMask,
        .dependencyFlags          = dependencyFlags,
        .memoryBarrierCount       = memoryBarrierCount,
        .pMemoryBarriers          = (APTR)pMemoryBarriers,
        .bufferMemoryBarrierCount = bufferMemoryBarrierCount,
        .pBufferMemoryBarriers    = (APTR)pBufferMemoryBarriers,
        .imageMemoryBarrierCount  = imageMemoryBarrierCount,
        .pImageMemoryBarriers     = (APTR)pImageMemoryBarriers,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdSetEvent(void *commandBuffer, void *event, uint32_t stageMask)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdSetEvent msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdSetEvent,
        .commandBuffer = commandBuffer,
        .event         = event,
        .stageMask     = stageMask,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdResetEvent(void *commandBuffer, void *event, uint32_t stageMask)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdResetEvent msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdResetEvent,
        .commandBuffer = commandBuffer,
        .event         = event,
        .stageMask     = stageMask,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdWaitEvents(void *commandBuffer, uint32_t eventCount,
                     const void *const *pEvents, uint32_t srcStageMask,
                     uint32_t dstStageMask, uint32_t memoryBarrierCount,
                     const void *pMemoryBarriers,
                     uint32_t bufferMemoryBarrierCount,
                     const void *pBufferMemoryBarriers,
                     uint32_t imageMemoryBarrierCount,
                     const void *pImageMemoryBarriers)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdWaitEvents msg = {
        .mID                      = vk_mbase + moHidd_Vulkan_CmdWaitEvents,
        .commandBuffer            = commandBuffer,
        .eventCount               = eventCount,
        .pEvents                  = (const APTR *)pEvents,
        .srcStageMask             = srcStageMask,
        .dstStageMask             = dstStageMask,
        .memoryBarrierCount       = memoryBarrierCount,
        .pMemoryBarriers          = (APTR)pMemoryBarriers,
        .bufferMemoryBarrierCount = bufferMemoryBarrierCount,
        .pBufferMemoryBarriers    = (APTR)pBufferMemoryBarriers,
        .imageMemoryBarrierCount  = imageMemoryBarrierCount,
        .pImageMemoryBarriers     = (APTR)pImageMemoryBarriers,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdWriteTimestamp(void *commandBuffer, uint32_t pipelineStage,
                         void *queryPool, uint32_t query)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdWriteTimestamp msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdWriteTimestamp,
        .commandBuffer = commandBuffer,
        .pipelineStage = pipelineStage,
        .queryPool     = queryPool,
        .query         = query,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}

void vkCmdResetQueryPool(void *commandBuffer, void *queryPool,
                         uint32_t firstQuery, uint32_t queryCount)
{
    vk_loader_init();
    if (!vk_obj)
        return;

    struct pHidd_Vulkan_CmdResetQueryPool msg = {
        .mID           = vk_mbase + moHidd_Vulkan_CmdResetQueryPool,
        .commandBuffer = commandBuffer,
        .queryPool     = queryPool,
        .firstQuery    = firstQuery,
        .queryCount    = queryCount,
    };
    OOP_DoMethod(vk_obj, (OOP_Msg)&msg);
}
