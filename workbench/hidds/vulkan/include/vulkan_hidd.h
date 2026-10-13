#ifndef HIDD_VULKAN_H
#define HIDD_VULKAN_H

/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan HIDD interface — abstract base class for Vulkan GPU drivers.
    Follows the same pattern as hidd/gallium.h.

    Hardware drivers (RADV, lavapipe, etc.) subclass this HIDD and
    override the methods with real implementations.

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

/* Known subclass IDs. NB: RADV (AMD Vulkan) and RadeonSI (AMD Gallium GL) are
 * DIFFERENT backends — the AMD Vulkan driver registers as radv, not radeonsi. */
#define CLID_Hidd_Vulkan_Lavapipe   "hidd.vulkan.lavapipe"
#define CLID_Hidd_Vulkan_Radv       "hidd.vulkan.radv"
#define CLID_Hidd_Vulkan_Radeonsi   "hidd.vulkan.radeonsi"  /* legacy alias */
#define CLID_Hidd_Vulkan_Venus      "hidd.vulkan.venus"

/* GPU capability flags (used in VulkanGPUInfo.flags) */
#define VULKAN_GPU_HEADLESS    (1 << 0)  /* no display outputs */
#define VULKAN_GPU_DISCRETE    (1 << 1)  /* dedicated GPU (not APU) */
#define VULKAN_GPU_INTEGRATED  (1 << 2)  /* APU / iGPU */
#define VULKAN_GPU_COMPUTE     (1 << 3)  /* compute-only capable */
#define VULKAN_GPU_VGPU        (1 << 4)  /* virtual GPU (SR-IOV / MxGPU) */
#define VULKAN_GPU_SOFTWARE    (1 << 5)  /* CPU/software Vulkan fallback */

/* CreateDevice flags */
#define VULKAN_DEVICE_COMPUTE_EXCLUSIVE  (1 << 0)  /* lock GPU exclusively */

/* Maximum GPUs in registry. Bumped 8 -> 32 to support inference racks
 * with many cards. Keep the public header and vulkan_intern.h in sync. */
#define VULKAN_MAX_GPUS     32

/* ================================================================
 * VulkanDriverCallbacks — full Vulkan compute driver interface.
 *
 * Provided by hardware drivers (RADV, lavapipe) at registration.
 * The vulkan.hidd base class dispatches to the preferred driver.
 *
 * All APTR handles are opaque Vulkan handles (VkDevice, VkBuffer, etc.)
 * The HIDD does not interpret them — just passes them through.
 *
 * driverData is the opaque context pointer passed at registration
 * (e.g. carddata / wsi_device).
 *
 * Return values: 0 = VK_SUCCESS, negative = VkResult error codes.
 * ================================================================ */
struct VulkanDriverCallbacks
{
    /* ---- ICD entry point ---- */
    /* Returns PFN_vkVoidFunction for any Vulkan function name.
     * Used for extension functions not in the callback table.
     * instance may be NULL for global functions. */
    APTR    (*GetInstanceProcAddr)(APTR instance, const char *pName,
                                   APTR driverData);
    APTR    (*GetDeviceProcAddr)(APTR device, const char *pName,
                                 APTR driverData);

    /* ---- Instance ---- */
    APTR    (*CreateInstance)(const char *appName, ULONG appVersion,
                              APTR driverData);
    void    (*DestroyInstance)(APTR instance, APTR driverData);
    int32_t (*EnumeratePhysicalDevices)(APTR instance, ULONG *pCount,
                                        APTR *pPhysicalDevices,
                                        APTR driverData);

    /* ---- Physical device queries ---- */
    void    (*GetPhysicalDeviceProperties)(APTR physDev, APTR pProperties,
                                            APTR driverData);
    void    (*GetPhysicalDeviceProperties2)(APTR physDev, APTR pProperties2,
                                             APTR driverData);
    void    (*GetPhysicalDeviceFeatures)(APTR physDev, APTR pFeatures,
                                          APTR driverData);
    void    (*GetPhysicalDeviceFeatures2)(APTR physDev, APTR pFeatures2,
                                           APTR driverData);
    void    (*GetPhysicalDeviceMemoryProperties)(APTR physDev,
                                                  APTR pMemProps,
                                                  APTR driverData);
    void    (*GetPhysicalDeviceQueueFamilyProperties)(APTR physDev,
                                                       ULONG *pCount,
                                                       APTR pQueueFamilyProps,
                                                       APTR driverData);
    int32_t (*EnumerateDeviceExtensionProperties)(APTR physDev,
                                                   const char *pLayerName,
                                                   ULONG *pCount,
                                                   APTR pProperties,
                                                   APTR driverData);

    /* ---- Device ---- */
    APTR    (*CreateDevice)(APTR physDev, APTR pCreateInfo, APTR driverData);
    void    (*DestroyDevice)(APTR device, APTR driverData);
    void    (*GetDeviceQueue)(APTR device, ULONG queueFamilyIndex,
                               ULONG queueIndex, APTR *pQueue,
                               APTR driverData);
    int32_t (*DeviceWaitIdle)(APTR device, APTR driverData);

    /* ---- Display (for windowed Vulkan, optional) ---- */
    void    (*DisplayResource)(APTR image, ULONG srcx, ULONG srcy,
                                struct RastPort *rp, ULONG dstx, ULONG dsty,
                                ULONG w, ULONG h, APTR driverData);

    /* ---- Memory ---- */
    int32_t (*AllocateMemory)(APTR device, uint64_t allocationSize,
                               ULONG memoryTypeIndex, APTR *pMemory,
                               APTR driverData);
    void    (*FreeMemory)(APTR device, APTR memory, APTR driverData);
    int32_t (*MapMemory)(APTR device, APTR memory, uint64_t offset,
                          uint64_t size, ULONG flags, APTR *ppData,
                          APTR driverData);
    void    (*UnmapMemory)(APTR device, APTR memory, APTR driverData);
    int32_t (*GetMemoryHostPointerPropertiesEXT)(APTR device,
                                                  ULONG handleType,
                                                  const APTR pHostPointer,
                                                  APTR pMemHostPtrProps,
                                                  APTR driverData);

    /* ---- Buffers ---- */
    int32_t (*CreateBuffer)(APTR device, uint64_t size, ULONG usage,
                             ULONG sharingMode, APTR *pBuffer,
                             APTR driverData);
    void    (*DestroyBuffer)(APTR device, APTR buffer, APTR driverData);
    int32_t (*BindBufferMemory)(APTR device, APTR buffer, APTR memory,
                                 uint64_t memoryOffset, APTR driverData);
    void    (*GetBufferMemoryRequirements)(APTR device, APTR buffer,
                                            APTR pMemReqs, APTR driverData);
    uint64_t (*GetBufferDeviceAddress)(APTR device, APTR pInfo,
                                        APTR driverData);

    /* ---- Descriptor sets ---- */
    int32_t (*CreateDescriptorSetLayout)(APTR device, APTR pCreateInfo,
                                          APTR *pSetLayout, APTR driverData);
    void    (*DestroyDescriptorSetLayout)(APTR device, APTR setLayout,
                                           APTR driverData);
    int32_t (*CreateDescriptorPool)(APTR device, APTR pCreateInfo,
                                     APTR *pDescriptorPool, APTR driverData);
    void    (*DestroyDescriptorPool)(APTR device, APTR descriptorPool,
                                      APTR driverData);
    int32_t (*AllocateDescriptorSets)(APTR device, APTR pAllocateInfo,
                                       APTR *pDescriptorSets,
                                       APTR driverData);
    void    (*UpdateDescriptorSets)(APTR device, ULONG writeCount,
                                     const APTR pDescriptorWrites,
                                     ULONG copyCount,
                                     const APTR pDescriptorCopies,
                                     APTR driverData);

    /* ---- Command pools and buffers ---- */
    int32_t (*CreateCommandPool)(APTR device, ULONG queueFamilyIndex,
                                  ULONG flags, APTR *pCommandPool,
                                  APTR driverData);
    void    (*DestroyCommandPool)(APTR device, APTR commandPool,
                                   APTR driverData);
    int32_t (*ResetCommandPool)(APTR device, APTR commandPool, ULONG flags,
                                 APTR driverData);
    int32_t (*AllocateCommandBuffers)(APTR device, APTR commandPool,
                                       ULONG level, ULONG count,
                                       APTR *pCommandBuffers,
                                       APTR driverData);
    void    (*FreeCommandBuffers)(APTR device, APTR commandPool,
                                   ULONG count, const APTR *pCommandBuffers,
                                   APTR driverData);
    int32_t (*BeginCommandBuffer)(APTR commandBuffer, ULONG flags,
                                   APTR driverData);
    int32_t (*EndCommandBuffer)(APTR commandBuffer, APTR driverData);

    /* ---- Queue submission ---- */
    int32_t (*QueueSubmit)(APTR queue, ULONG submitCount,
                            const APTR pSubmits, APTR fence,
                            APTR driverData);
    int32_t (*QueueWaitIdle)(APTR queue, APTR driverData);

    /* ---- Synchronization — fences ---- */
    int32_t (*CreateFence)(APTR device, ULONG flags, APTR *pFence,
                            APTR driverData);
    void    (*DestroyFence)(APTR device, APTR fence, APTR driverData);
    int32_t (*WaitForFences)(APTR device, ULONG fenceCount,
                              const APTR *pFences, ULONG waitAll,
                              uint64_t timeout, APTR driverData);
    int32_t (*ResetFences)(APTR device, ULONG fenceCount,
                            const APTR *pFences, APTR driverData);
    int32_t (*GetFenceStatus)(APTR device, APTR fence, APTR driverData);

    /* ---- Synchronization — semaphores ---- */
    int32_t (*CreateSemaphore)(APTR device, APTR pCreateInfo,
                                APTR *pSemaphore, APTR driverData);
    void    (*DestroySemaphore)(APTR device, APTR semaphore,
                                 APTR driverData);

    /* ---- Synchronization — events ---- */
    int32_t (*CreateEvent)(APTR device, APTR *pEvent, APTR driverData);
    void    (*DestroyEvent)(APTR device, APTR event, APTR driverData);
    int32_t (*SetEvent)(APTR device, APTR event, APTR driverData);
    int32_t (*ResetEvent)(APTR device, APTR event, APTR driverData);

    /* ---- Shader / pipeline ---- */
    int32_t (*CreateShaderModule)(APTR device, const ULONG *pCode,
                                   ULONG codeSize, APTR *pShaderModule,
                                   APTR driverData);
    void    (*DestroyShaderModule)(APTR device, APTR shaderModule,
                                    APTR driverData);
    int32_t (*CreatePipelineLayout)(APTR device, ULONG setLayoutCount,
                                     const APTR *pSetLayouts,
                                     ULONG pushConstantRangeCount,
                                     const APTR pPushConstantRanges,
                                     APTR *pPipelineLayout, APTR driverData);
    void    (*DestroyPipelineLayout)(APTR device, APTR pipelineLayout,
                                      APTR driverData);
    int32_t (*CreateComputePipelines)(APTR device, ULONG createInfoCount,
                                       const APTR pCreateInfos,
                                       APTR *pPipelines, APTR driverData);
    void    (*DestroyPipeline)(APTR device, APTR pipeline, APTR driverData);

    /* ---- Query pools ---- */
    int32_t (*CreateQueryPool)(APTR device, APTR pCreateInfo,
                                APTR *pQueryPool, APTR driverData);
    void    (*DestroyQueryPool)(APTR device, APTR queryPool,
                                 APTR driverData);
    int32_t (*GetQueryPoolResults)(APTR device, APTR queryPool,
                                    ULONG firstQuery, ULONG queryCount,
                                    ULONG dataSize, APTR pData,
                                    uint64_t stride, ULONG flags,
                                    APTR driverData);
    void    (*ResetQueryPool)(APTR device, APTR queryPool,
                               ULONG firstQuery, ULONG queryCount,
                               APTR driverData);

    /* ---- Command buffer recording — compute commands ---- */
    void    (*CmdBindPipeline)(APTR cmdBuf, ULONG bindPoint, APTR pipeline,
                                APTR driverData);
    void    (*CmdBindDescriptorSets)(APTR cmdBuf, ULONG bindPoint,
                                      APTR layout, ULONG firstSet,
                                      ULONG setCount, const APTR *pSets,
                                      ULONG dynamicOffsetCount,
                                      const ULONG *pDynamicOffsets,
                                      APTR driverData);
    void    (*CmdPushConstants)(APTR cmdBuf, APTR layout,
                                 ULONG stageFlags, ULONG offset,
                                 ULONG size, const APTR pValues,
                                 APTR driverData);
    void    (*CmdDispatch)(APTR cmdBuf, ULONG groupCountX,
                            ULONG groupCountY, ULONG groupCountZ,
                            APTR driverData);

    /* ---- Command buffer recording — transfer commands ---- */
    void    (*CmdCopyBuffer)(APTR cmdBuf, APTR srcBuffer, APTR dstBuffer,
                              ULONG regionCount, const APTR pRegions,
                              APTR driverData);
    void    (*CmdFillBuffer)(APTR cmdBuf, APTR dstBuffer,
                              uint64_t dstOffset, uint64_t size,
                              ULONG data, APTR driverData);

    /* ---- Command buffer recording — synchronization ---- */
    void    (*CmdPipelineBarrier)(APTR cmdBuf,
                                   ULONG srcStageMask, ULONG dstStageMask,
                                   ULONG dependencyFlags,
                                   ULONG memoryBarrierCount,
                                   const APTR pMemoryBarriers,
                                   ULONG bufferMemoryBarrierCount,
                                   const APTR pBufferMemoryBarriers,
                                   ULONG imageMemoryBarrierCount,
                                   const APTR pImageMemoryBarriers,
                                   APTR driverData);
    void    (*CmdSetEvent)(APTR cmdBuf, APTR event, ULONG stageMask,
                            APTR driverData);
    void    (*CmdResetEvent)(APTR cmdBuf, APTR event, ULONG stageMask,
                              APTR driverData);
    void    (*CmdWaitEvents)(APTR cmdBuf, ULONG eventCount,
                              const APTR *pEvents,
                              ULONG srcStageMask, ULONG dstStageMask,
                              ULONG memBarrierCount,
                              const APTR pMemBarriers,
                              ULONG bufBarrierCount,
                              const APTR pBufBarriers,
                              ULONG imgBarrierCount,
                              const APTR pImgBarriers,
                              APTR driverData);

    /* ---- Command buffer recording — queries ---- */
    void    (*CmdWriteTimestamp)(APTR cmdBuf, ULONG pipelineStage,
                                  APTR queryPool, ULONG query,
                                  APTR driverData);
    void    (*CmdResetQueryPool)(APTR cmdBuf, APTR queryPool,
                                  ULONG firstQuery, ULONG queryCount,
                                  APTR driverData);

    /* ---- Instance-level queries ---- */
    int32_t (*EnumerateInstanceVersion)(ULONG *pApiVersion,
                                         APTR driverData);
    int32_t (*EnumerateInstanceExtensionProperties)(const char *pLayerName,
                                                      ULONG *pCount,
                                                      APTR pProperties,
                                                      APTR driverData);
    int32_t (*EnumerateInstanceLayerProperties)(ULONG *pCount,
                                                 APTR pProperties,
                                                 APTR driverData);
};

/*
 * Public GPU registration function.
 * Called by driver HIDDs (e.g. radeonsi) at init time to register
 * a Vulkan-capable GPU with the Vulkan HIDD's registry.
 *
 * callbacks: driver dispatch table (required — vulkan.hidd routes here)
 * driver_data: opaque pointer passed to every callback (e.g. carddata)
 *
 * Returns slot index (0..VULKAN_MAX_GPUS-1) or -1 on failure.
 */
int32_t VulkanHidd_RegisterGPU(const char *class_id,
                                OOP_Object *pci_device,
                                uint32_t vendor_id,
                                uint32_t device_id,
                                uint32_t flags,
                                uint64_t vram_size,
                                const char *name,
                                int32_t priority,
                                const struct VulkanDriverCallbacks *callbacks,
                                APTR driver_data,
                                uint8_t pci_bus,
                                uint8_t pci_dev_num,
                                uint8_t pci_func);

/*
 * Coarse cross-call substrate-load tracking.
 *
 * Some driver HIDDs (radeonsi) have multiple independent code paths that
 * each want to ensure the card's PSP firmware is loaded before they touch
 * compute/graphics rings — most notably HIDD AccelInit and the RADV
 * winsys-create entry. These flags let those paths agree on a single
 * source of truth across the library boundary, so we don't redundantly
 * reload firmware on a card that's already alive (which on real silicon
 * can wedge in-flight rings).
 *
 * The flag is opaque to vulkan.hidd — it's just one bit per registered
 * card. Drivers must still hardware-probe before fully trusting the
 * LOADED state, because warm reboots / VFIO reattach can leave the bit
 * set while the SOC has actually been reset under us.
 *
 * Returns TRUE on a successful slot match; FALSE if no GPU with the
 * given (BDF + vendor + device) is currently registered.
 */
BOOL VulkanHidd_SetGPUPSPLoaded(uint8_t pci_bus,
                                uint8_t pci_dev_num,
                                uint8_t pci_func,
                                uint32_t vendor_id,
                                uint32_t device_id,
                                BOOL loaded);
BOOL VulkanHidd_IsGPUPSPLoaded(uint8_t pci_bus,
                               uint8_t pci_dev_num,
                               uint8_t pci_func,
                               uint32_t vendor_id,
                               uint32_t device_id);

#define HiddVulkanAttrBase __IHidd_Vulkan

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddVulkanAttrBase;
#endif

/*
 * Vulkan HIDD interface version.
 * Bump when the method table or struct layouts change.
 */
#define VULKAN_INTERFACE_VERSION    2

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

    /* Command recording — compute */
    moHidd_Vulkan_CmdBindPipeline,
    moHidd_Vulkan_CmdBindDescriptorSets,
    moHidd_Vulkan_CmdPushConstants,
    moHidd_Vulkan_CmdDispatch,

    /* Command recording — transfer */
    moHidd_Vulkan_CmdCopyBuffer,
    moHidd_Vulkan_CmdFillBuffer,

    /* Command recording — synchronization */
    moHidd_Vulkan_CmdPipelineBarrier,
    moHidd_Vulkan_CmdSetEvent,
    moHidd_Vulkan_CmdResetEvent,
    moHidd_Vulkan_CmdWaitEvents,

    /* Command recording — queries */
    moHidd_Vulkan_CmdWriteTimestamp,
    moHidd_Vulkan_CmdResetQueryPool,

    /* GPU registry management */
    moHidd_Vulkan_EnumerateGPUs,
    moHidd_Vulkan_SetGPUPriority,

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
    aoHidd_Vulkan_GPUCount,

    num_Hidd_Vulkan_Attrs
};

#define aHidd_Vulkan_InterfaceVersion \
    (HiddVulkanAttrBase + aoHidd_Vulkan_InterfaceVersion)

#define aHidd_Vulkan_GPUCount \
    (HiddVulkanAttrBase + aoHidd_Vulkan_GPUCount)

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

/* ---- Command recording — compute ---- */

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

/* ---- Command recording — transfer ---- */

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

/* ---- Command recording — synchronization ---- */

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

/* ---- Command recording — queries ---- */

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

/* ---- GPU registry management ---- */

struct VulkanGPUInfo
{
    uint32_t        vendor_id;
    uint32_t        device_id;
    uint32_t        flags;
    int32_t         priority;
    uint64_t        vram_size;
    uint8_t         pci_bus;
    uint8_t         pci_dev;
    uint8_t         pci_func;
    uint8_t         pad;
    char            name[64];
    char            class_id[64];
};

struct pHidd_Vulkan_EnumerateGPUs
{
    STACKED OOP_MethodID            mID;
    STACKED struct VulkanGPUInfo   *gpu_info;
    STACKED ULONG                   max_count;
};

struct pHidd_Vulkan_SetGPUPriority
{
    STACKED OOP_MethodID    mID;
    STACKED ULONG           gpu_index;
    STACKED LONG            priority;
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
