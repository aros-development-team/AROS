#ifndef HIDD_VULKAN_H
#define HIDD_VULKAN_H

/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan HIDD interface - abstract base class for Vulkan GPU drivers.
    Follows the same pattern as hidd/gallium.h.

    A display driver with a Vulkan driver for its card subclasses this
    HIDD, overrides the methods, and returns an object of its subclass
    from CreateObject when asked for this class.

    The HIDD provides a complete Vulkan 1.2 compute interface:
     - Instance/device lifecycle
     - Physical device property queries
     - Memory allocation and mapping
     - Buffer creation and binding
     - Descriptor set management
     - Pipeline/shader management
     - Command buffer recording
     - Synchronization (fences, semaphores, events)
     - Query pools
     - ICD-style GetProcAddr for extensions
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef _STDINT_H_
#   include <stdint.h>
#endif

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

/* Vulkan HIDD interface */
#define CLID_Hidd_Vulkan            "hidd.vulkan"
#define IID_Hidd_Vulkan             "hidd.vulkan"

/*
 * Driver subclasses: hidd.vulkan.<driver>. A display driver's CreateObject,
 * asked for the hidd.vulkan class, returns an object of its subclass; the
 * software driver is made directly, as the fallback.
 *
 * A subclass overrides GetInstanceProcAddr (and GetDeviceProcAddr) to return
 * its ICD's entry points. The base class implements every other method
 * through them, so a driver needs nothing else to serve the loader and zink.
 */

#define HiddVulkanAttrBase __IHidd_Vulkan

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddVulkanAttrBase;
#endif

/*
 * Vulkan HIDD interface version.
 * Bump when the method table or struct layouts change.
 */
#define VULKAN_INTERFACE_VERSION    3

/* ---- Methods ---- */
enum
{
    /* Instance lifecycle */
    moHidd_Vulkan_CreateInstance = 0,
    moHidd_Vulkan_DestroyInstance,

    /* Physical device enumeration */
    moHidd_Vulkan_EnumeratePhysicalDevices,

    /* Physical device queries */
    moHidd_Vulkan_GetPhysicalDeviceProperties,
    moHidd_Vulkan_GetPhysicalDeviceProperties2,
    moHidd_Vulkan_GetPhysicalDeviceFeatures,
    moHidd_Vulkan_GetPhysicalDeviceFeatures2,
    moHidd_Vulkan_GetPhysicalDeviceMemoryProperties,
    moHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties,
    moHidd_Vulkan_EnumerateDeviceExtensionProperties,

    /* Device lifecycle */
    moHidd_Vulkan_CreateDevice,
    moHidd_Vulkan_DestroyDevice,
    moHidd_Vulkan_GetDeviceQueue,
    moHidd_Vulkan_DeviceWaitIdle,

    /* Display (optional) */
    moHidd_Vulkan_DisplayResource,

    /* Memory */
    moHidd_Vulkan_AllocateMemory,
    moHidd_Vulkan_FreeMemory,
    moHidd_Vulkan_MapMemory,
    moHidd_Vulkan_UnmapMemory,
    moHidd_Vulkan_GetMemoryHostPointerPropertiesEXT,

    /* Buffers */
    moHidd_Vulkan_CreateBuffer,
    moHidd_Vulkan_DestroyBuffer,
    moHidd_Vulkan_BindBufferMemory,
    moHidd_Vulkan_GetBufferMemoryRequirements,
    moHidd_Vulkan_GetBufferDeviceAddress,

    /* Descriptor sets */
    moHidd_Vulkan_CreateDescriptorSetLayout,
    moHidd_Vulkan_DestroyDescriptorSetLayout,
    moHidd_Vulkan_CreateDescriptorPool,
    moHidd_Vulkan_DestroyDescriptorPool,
    moHidd_Vulkan_AllocateDescriptorSets,
    moHidd_Vulkan_UpdateDescriptorSets,

    /* Command pools and buffers */
    moHidd_Vulkan_CreateCommandPool,
    moHidd_Vulkan_DestroyCommandPool,
    moHidd_Vulkan_ResetCommandPool,
    moHidd_Vulkan_AllocateCommandBuffers,
    moHidd_Vulkan_FreeCommandBuffers,
    moHidd_Vulkan_BeginCommandBuffer,
    moHidd_Vulkan_EndCommandBuffer,

    /* Queue submission */
    moHidd_Vulkan_QueueSubmit,
    moHidd_Vulkan_QueueWaitIdle,

    /* Fences */
    moHidd_Vulkan_CreateFence,
    moHidd_Vulkan_DestroyFence,
    moHidd_Vulkan_WaitForFences,
    moHidd_Vulkan_ResetFences,
    moHidd_Vulkan_GetFenceStatus,

    /* Semaphores */
    moHidd_Vulkan_CreateSemaphore,
    moHidd_Vulkan_DestroySemaphore,

    /* Events */
    moHidd_Vulkan_CreateEvent,
    moHidd_Vulkan_DestroyEvent,
    moHidd_Vulkan_SetEvent,
    moHidd_Vulkan_ResetEvent,

    /* Shader / pipeline */
    moHidd_Vulkan_CreateShaderModule,
    moHidd_Vulkan_DestroyShaderModule,
    moHidd_Vulkan_CreatePipelineLayout,
    moHidd_Vulkan_DestroyPipelineLayout,
    moHidd_Vulkan_CreateComputePipelines,
    moHidd_Vulkan_DestroyPipeline,

    /* Query pools */
    moHidd_Vulkan_CreateQueryPool,
    moHidd_Vulkan_DestroyQueryPool,
    moHidd_Vulkan_GetQueryPoolResults,
    moHidd_Vulkan_ResetQueryPool,

    /* Command recording - compute */
    moHidd_Vulkan_CmdBindPipeline,
    moHidd_Vulkan_CmdBindDescriptorSets,
    moHidd_Vulkan_CmdPushConstants,
    moHidd_Vulkan_CmdDispatch,

    /* Command recording - transfer */
    moHidd_Vulkan_CmdCopyBuffer,
    moHidd_Vulkan_CmdFillBuffer,

    /* Command recording - synchronization */
    moHidd_Vulkan_CmdPipelineBarrier,
    moHidd_Vulkan_CmdSetEvent,
    moHidd_Vulkan_CmdResetEvent,
    moHidd_Vulkan_CmdWaitEvents,

    /* Command recording - queries */
    moHidd_Vulkan_CmdWriteTimestamp,
    moHidd_Vulkan_CmdResetQueryPool,

    /* ICD-style function lookup */
    moHidd_Vulkan_GetInstanceProcAddr,
    moHidd_Vulkan_GetDeviceProcAddr,

    /* Instance-level queries */
    moHidd_Vulkan_EnumerateInstanceVersion,
    moHidd_Vulkan_EnumerateInstanceExtensionProperties,

    NUM_VULKAN_METHODS
};

/* ---- Attributes ---- */
enum
{
    aoHidd_Vulkan_InterfaceVersion = 0,

    num_Hidd_Vulkan_Attrs
};

#define aHidd_Vulkan_InterfaceVersion \
    (HiddVulkanAttrBase + aoHidd_Vulkan_InterfaceVersion)

#define IS_VULKAN_ATTR(attr, idx) \
    (((idx) = (attr) - HiddVulkanAttrBase) < num_Hidd_Vulkan_Attrs)

/* ====================================================================
 * Method parameter structures
 * ==================================================================== */

/* ---- Instance ---- */

struct pHidd_Vulkan_CreateInstance
{
    STACKED OOP_MethodID    mID;
    STACKED const char     *applicationName;
    STACKED ULONG           applicationVersion;
};

struct pHidd_Vulkan_DestroyInstance
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            instance;
};

struct pHidd_Vulkan_EnumeratePhysicalDevices
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            instance;
    STACKED ULONG          *pPhysicalDeviceCount;
    STACKED APTR           *pPhysicalDevices;   /* output array, may be NULL */
};

/* ---- Physical device queries ---- */

struct pHidd_Vulkan_GetPhysicalDeviceProperties
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pProperties;    /* VkPhysicalDeviceProperties* */
};

struct pHidd_Vulkan_GetPhysicalDeviceProperties2
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pProperties;    /* VkPhysicalDeviceProperties2* */
};

struct pHidd_Vulkan_GetPhysicalDeviceFeatures
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pFeatures;      /* VkPhysicalDeviceFeatures* */
};

struct pHidd_Vulkan_GetPhysicalDeviceFeatures2
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pFeatures;      /* VkPhysicalDeviceFeatures2* */
};

struct pHidd_Vulkan_GetPhysicalDeviceMemoryProperties
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pMemoryProperties;
};

struct pHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED ULONG          *pQueueFamilyPropertyCount;
    STACKED APTR            pQueueFamilyProperties; /* may be NULL */
};

struct pHidd_Vulkan_EnumerateDeviceExtensionProperties
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED const char     *pLayerName;     /* may be NULL */
    STACKED ULONG          *pPropertyCount;
    STACKED APTR            pProperties;    /* may be NULL */
};

/* ---- Device ---- */

struct pHidd_Vulkan_CreateDevice
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            physicalDevice;
    STACKED APTR            pCreateInfo;    /* VkDeviceCreateInfo* */
};

struct pHidd_Vulkan_DestroyDevice
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
};

struct pHidd_Vulkan_GetDeviceQueue
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           queueFamilyIndex;
    STACKED ULONG           queueIndex;
    STACKED APTR           *pQueue;         /* output */
};

struct pHidd_Vulkan_DeviceWaitIdle
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
};

/* ---- Display ---- */

struct pHidd_Vulkan_DisplayResource
{
    STACKED OOP_MethodID            mID;
    STACKED APTR                    image;
    STACKED ULONG                   srcx;
    STACKED ULONG                   srcy;
    STACKED struct RastPort        *rastPort;
    STACKED ULONG                   dstx;
    STACKED ULONG                   dsty;
    STACKED ULONG                   width;
    STACKED ULONG                   height;
};

/* ---- Memory ---- */

struct pHidd_Vulkan_AllocateMemory
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED uint64_t        allocationSize;
    STACKED ULONG           memoryTypeIndex;
    STACKED APTR           *pMemory;        /* output */
};

struct pHidd_Vulkan_FreeMemory
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            memory;
};

struct pHidd_Vulkan_MapMemory
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            memory;
    STACKED uint64_t        offset;
    STACKED uint64_t        size;
    STACKED ULONG           flags;
    STACKED APTR           *ppData;         /* output */
};

struct pHidd_Vulkan_UnmapMemory
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            memory;
};

struct pHidd_Vulkan_GetMemoryHostPointerPropertiesEXT
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           handleType;
    STACKED const APTR      pHostPointer;
    STACKED APTR            pMemoryHostPointerProperties;   /* output */
};

/* ---- Buffers ---- */

struct pHidd_Vulkan_CreateBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED uint64_t        size;
    STACKED ULONG           usage;
    STACKED ULONG           sharingMode;
    STACKED APTR           *pBuffer;        /* output */
};

struct pHidd_Vulkan_DestroyBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            buffer;
};

struct pHidd_Vulkan_BindBufferMemory
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            buffer;
    STACKED APTR            memory;
    STACKED uint64_t        memoryOffset;
};

struct pHidd_Vulkan_GetBufferMemoryRequirements
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            buffer;
    STACKED APTR            pMemoryRequirements;    /* output */
};

struct pHidd_Vulkan_GetBufferDeviceAddress
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pInfo;  /* VkBufferDeviceAddressInfo* */
};

/* ---- Descriptor sets ---- */

struct pHidd_Vulkan_CreateDescriptorSetLayout
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pCreateInfo;
    STACKED APTR           *pSetLayout;     /* output */
};

struct pHidd_Vulkan_DestroyDescriptorSetLayout
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            descriptorSetLayout;
};

struct pHidd_Vulkan_CreateDescriptorPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pCreateInfo;
    STACKED APTR           *pDescriptorPool;    /* output */
};

struct pHidd_Vulkan_DestroyDescriptorPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            descriptorPool;
};

struct pHidd_Vulkan_AllocateDescriptorSets
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pAllocateInfo;
    STACKED APTR           *pDescriptorSets;    /* output */
};

struct pHidd_Vulkan_UpdateDescriptorSets
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           descriptorWriteCount;
    STACKED const APTR      pDescriptorWrites;
    STACKED ULONG           descriptorCopyCount;
    STACKED const APTR      pDescriptorCopies;
};

/* ---- Command pools and buffers ---- */

struct pHidd_Vulkan_CreateCommandPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           queueFamilyIndex;
    STACKED ULONG           flags;
    STACKED APTR           *pCommandPool;   /* output */
};

struct pHidd_Vulkan_DestroyCommandPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            commandPool;
};

struct pHidd_Vulkan_ResetCommandPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            commandPool;
    STACKED ULONG           flags;
};

struct pHidd_Vulkan_AllocateCommandBuffers
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            commandPool;
    STACKED ULONG           level;
    STACKED ULONG           count;
    STACKED APTR           *pCommandBuffers;    /* output */
};

struct pHidd_Vulkan_FreeCommandBuffers
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            commandPool;
    STACKED ULONG           count;
    STACKED const APTR     *pCommandBuffers;
};

struct pHidd_Vulkan_BeginCommandBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           flags;
};

struct pHidd_Vulkan_EndCommandBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
};

/* ---- Queue submission ---- */

struct pHidd_Vulkan_QueueSubmit
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            queue;
    STACKED ULONG           submitCount;
    STACKED const APTR      pSubmits;       /* VkSubmitInfo* */
    STACKED APTR            fence;
};

struct pHidd_Vulkan_QueueWaitIdle
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            queue;
};

/* ---- Fences ---- */

struct pHidd_Vulkan_CreateFence
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           flags;
    STACKED APTR           *pFence;         /* output */
};

struct pHidd_Vulkan_DestroyFence
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            fence;
};

struct pHidd_Vulkan_WaitForFences
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           fenceCount;
    STACKED const APTR     *pFences;
    STACKED ULONG           waitAll;
    STACKED uint64_t        timeout;
};

struct pHidd_Vulkan_ResetFences
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           fenceCount;
    STACKED const APTR     *pFences;
};

struct pHidd_Vulkan_GetFenceStatus
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            fence;
};

/* ---- Semaphores ---- */

struct pHidd_Vulkan_CreateSemaphore
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pCreateInfo;
    STACKED APTR           *pSemaphore;     /* output */
};

struct pHidd_Vulkan_DestroySemaphore
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            semaphore;
};

/* ---- Events ---- */

struct pHidd_Vulkan_CreateEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR           *pEvent;         /* output */
};

struct pHidd_Vulkan_DestroyEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            event;
};

struct pHidd_Vulkan_SetEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            event;
};

struct pHidd_Vulkan_ResetEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            event;
};

/* ---- Shader / pipeline ---- */

struct pHidd_Vulkan_CreateShaderModule
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED const ULONG    *pCode;
    STACKED ULONG           codeSize;
    STACKED APTR           *pShaderModule;  /* output */
};

struct pHidd_Vulkan_DestroyShaderModule
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            shaderModule;
};

struct pHidd_Vulkan_CreatePipelineLayout
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           setLayoutCount;
    STACKED const APTR     *pSetLayouts;
    STACKED ULONG           pushConstantRangeCount;
    STACKED const APTR      pPushConstantRanges;
    STACKED APTR           *pPipelineLayout;    /* output */
};

struct pHidd_Vulkan_DestroyPipelineLayout
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pipelineLayout;
};

struct pHidd_Vulkan_CreateComputePipelines
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED ULONG           createInfoCount;
    STACKED const APTR      pCreateInfos;
    STACKED APTR           *pPipelines;     /* output */
};

struct pHidd_Vulkan_DestroyPipeline
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pipeline;
};

/* ---- Query pools ---- */

struct pHidd_Vulkan_CreateQueryPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            pCreateInfo;
    STACKED APTR           *pQueryPool;     /* output */
};

struct pHidd_Vulkan_DestroyQueryPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            queryPool;
};

struct pHidd_Vulkan_GetQueryPoolResults
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            queryPool;
    STACKED ULONG           firstQuery;
    STACKED ULONG           queryCount;
    STACKED ULONG           dataSize;
    STACKED APTR            pData;
    STACKED uint64_t        stride;
    STACKED ULONG           flags;
};

struct pHidd_Vulkan_ResetQueryPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;
    STACKED APTR            queryPool;
    STACKED ULONG           firstQuery;
    STACKED ULONG           queryCount;
};

/* ---- Command recording - compute ---- */

struct pHidd_Vulkan_CmdBindPipeline
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           pipelineBindPoint;
    STACKED APTR            pipeline;
};

struct pHidd_Vulkan_CmdBindDescriptorSets
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           pipelineBindPoint;
    STACKED APTR            layout;
    STACKED ULONG           firstSet;
    STACKED ULONG           descriptorSetCount;
    STACKED const APTR     *pDescriptorSets;
    STACKED ULONG           dynamicOffsetCount;
    STACKED const ULONG    *pDynamicOffsets;
};

struct pHidd_Vulkan_CmdPushConstants
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            layout;
    STACKED ULONG           stageFlags;
    STACKED ULONG           offset;
    STACKED ULONG           size;
    STACKED const APTR      pValues;
};

struct pHidd_Vulkan_CmdDispatch
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           groupCountX;
    STACKED ULONG           groupCountY;
    STACKED ULONG           groupCountZ;
};

/* ---- Command recording - transfer ---- */

struct pHidd_Vulkan_CmdCopyBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            srcBuffer;
    STACKED APTR            dstBuffer;
    STACKED ULONG           regionCount;
    STACKED const APTR      pRegions;   /* VkBufferCopy* */
};

struct pHidd_Vulkan_CmdFillBuffer
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            dstBuffer;
    STACKED uint64_t        dstOffset;
    STACKED uint64_t        size;
    STACKED ULONG           data;
};

/* ---- Command recording - synchronization ---- */

struct pHidd_Vulkan_CmdPipelineBarrier
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           srcStageMask;
    STACKED ULONG           dstStageMask;
    STACKED ULONG           dependencyFlags;
    STACKED ULONG           memoryBarrierCount;
    STACKED const APTR      pMemoryBarriers;
    STACKED ULONG           bufferMemoryBarrierCount;
    STACKED const APTR      pBufferMemoryBarriers;
    STACKED ULONG           imageMemoryBarrierCount;
    STACKED const APTR      pImageMemoryBarriers;
};

struct pHidd_Vulkan_CmdSetEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            event;
    STACKED ULONG           stageMask;
};

struct pHidd_Vulkan_CmdResetEvent
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            event;
    STACKED ULONG           stageMask;
};

struct pHidd_Vulkan_CmdWaitEvents
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           eventCount;
    STACKED const APTR     *pEvents;
    STACKED ULONG           srcStageMask;
    STACKED ULONG           dstStageMask;
    STACKED ULONG           memoryBarrierCount;
    STACKED const APTR      pMemoryBarriers;
    STACKED ULONG           bufferMemoryBarrierCount;
    STACKED const APTR      pBufferMemoryBarriers;
    STACKED ULONG           imageMemoryBarrierCount;
    STACKED const APTR      pImageMemoryBarriers;
};

/* ---- Command recording - queries ---- */

struct pHidd_Vulkan_CmdWriteTimestamp
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED ULONG           pipelineStage;
    STACKED APTR            queryPool;
    STACKED ULONG           query;
};

struct pHidd_Vulkan_CmdResetQueryPool
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            commandBuffer;
    STACKED APTR            queryPool;
    STACKED ULONG           firstQuery;
    STACKED ULONG           queryCount;
};

/* ---- ICD-style function lookup ---- */

struct pHidd_Vulkan_GetInstanceProcAddr
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            instance;       /* VkInstance, may be NULL */
    STACKED const char     *pName;
};

struct pHidd_Vulkan_GetDeviceProcAddr
{
    STACKED OOP_MethodID    mID;
    STACKED APTR            device;         /* VkDevice */
    STACKED const char     *pName;
};

/* ---- Instance-level queries ---- */

struct pHidd_Vulkan_EnumerateInstanceVersion
{
    STACKED OOP_MethodID    mID;
    STACKED ULONG          *pApiVersion;    /* output */
};

struct pHidd_Vulkan_EnumerateInstanceExtensionProperties
{
    STACKED OOP_MethodID    mID;
    STACKED const char     *pLayerName;     /* may be NULL */
    STACKED ULONG          *pPropertyCount;
    STACKED APTR            pProperties;    /* may be NULL */
};

#endif /* HIDD_VULKAN_H */
