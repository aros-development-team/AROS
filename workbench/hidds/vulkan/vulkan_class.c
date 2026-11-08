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

/****************************************************************************
    NAME
        --background_vulkan_hidd--

    LOCATION
        hidd.vulkan

    FUNCTION
        Provides OOP methods that dispatch Vulkan requests through a
        driver-supplied Vulkan ICD. Driver subclasses supply
        GetInstanceProcAddr; the base implementation of that method
        returns NULL.

    NOTES
        A Vulkan HIDD object requires the
        aHidd_Vulkan_InterfaceVersion creation tag set to
        VULKAN_INTERFACE_VERSION. Root.New rejects missing and mismatched
        values. Method messages and IDs are defined in vulkan_hidd.h.

        The base class caches the ICD's vkGetInstanceProcAddr pointer
        and remembers the first successfully created VkInstance for
        subsequent command lookup. It clears that instance pointer
        when the same instance is destroyed.

        Methods with a VkResult return expose it as IPTR. If an entry
        point is unavailable, methods returning VkResult normally
        return VK_ERROR_INITIALIZATION_FAILED. Methods returning
        VOID normally skip an unavailable entry point. Individual
        methods document exceptions to these defaults.

    SEE ALSO
        moHidd_Vulkan_GetInstanceProcAddr,
        moHidd_Vulkan_CreateInstance

*****************************************************************************/

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

/*
 * Rejects object construction unless the caller supplies the exact
 * VULKAN_INTERFACE_VERSION through aHidd_Vulkan_InterfaceVersion. Delegates
 * construction to the superclass and initializes the new object's ICD
 * function cache and instance pointer to NULL.
 */
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

/*
 * Returns VULKAN_INTERFACE_VERSION for the interface-version attribute.
 * Delegates other attribute requests to the superclass.
 */
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

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateInstance

    SYNOPSIS
        struct pHidd_Vulkan_CreateInstance

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a Vulkan instance using the driver's ICD entry point.

    INPUTS
        applicationName - Application name passed to VkApplicationInfo.
        applicationVersion - Application version passed to
            VkApplicationInfo.

    RESULT
        Vulkan object handle, or NULL if creation fails or the entry point is
        absent.

    NOTES
        The base implementation creates a Vulkan 1.2 VkApplicationInfo and
        retains the first successfully created instance for subsequent command
        resolution. It does not keep a list of instances. DestroyInstance
        clears that pointer only when destroying the retained instance.

        The base method supplies VkInstanceCreateInfo with no instance
        extensions or layers enabled and passes NULL allocation callbacks.
        The message provides no extension or layer lists.

    SEE ALSO
        moHidd_Vulkan_DestroyInstance, moHidd_Vulkan_GetInstanceProcAddr

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyInstance

    SYNOPSIS
        struct pHidd_Vulkan_DestroyInstance

    LOCATION
        hidd.vulkan

    FUNCTION
        Releases a Vulkan instance through the driver's ICD.

    INPUTS
        instance - Vulkan instance handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    NOTES
        Destroying the retained instance clears the base class's instance
        pointer. Other instances do not replace that pointer.

        The base method clears its cached instance pointer for a matching
        handle even when the ICD destroy entry point is unavailable.

    SEE ALSO
        moHidd_Vulkan_CreateInstance

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyInstance)
{
    struct HiddVulkanData *data = OOP_INST_DATA(cl, o);

    VKVOID(DestroyInstance, H(VkInstance, msg->instance), NULL);
    if (data->instance == msg->instance)
        data->instance = NULL;
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_EnumeratePhysicalDevices

    SYNOPSIS
        struct pHidd_Vulkan_EnumeratePhysicalDevices

    LOCATION
        hidd.vulkan

    FUNCTION
        Enumerates physical devices associated with an instance.

    INPUTS
        instance - Vulkan instance handle.
        pPhysicalDeviceCount - Pointer to the physical-device count.
        pPhysicalDevices - Optional output array of physical-device
            handles.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumeratePhysicalDevices)
{
    VKCALL(EnumeratePhysicalDevices, H(VkInstance, msg->instance),
           (uint32_t *)msg->pPhysicalDeviceCount, HA(VkPhysicalDevice, msg->pPhysicalDevices));
}

/* ==================================================================
 * Physical device queries
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceProperties

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceProperties

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads the properties of a physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pProperties - Caller-provided output properties structure or
            array.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties)
{
    VKVOID(GetPhysicalDeviceProperties, H(VkPhysicalDevice, msg->physicalDevice), msg->pProperties);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceProperties2

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceProperties2

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads extended physical-device properties.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pProperties - Caller-provided output properties structure or
            array.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceProperties2)
{
    VKVOID(GetPhysicalDeviceProperties2, H(VkPhysicalDevice, msg->physicalDevice), msg->pProperties);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceFeatures

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceFeatures

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads the supported features of a physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pFeatures - Caller-provided output features structure.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures)
{
    VKVOID(GetPhysicalDeviceFeatures, H(VkPhysicalDevice, msg->physicalDevice), msg->pFeatures);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceFeatures2

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceFeatures2

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads extended physical-device features.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pFeatures - Caller-provided output features structure.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceFeatures2)
{
    VKVOID(GetPhysicalDeviceFeatures2, H(VkPhysicalDevice, msg->physicalDevice), msg->pFeatures);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceMemoryProperties

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceMemoryProperties

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads the memory heaps and types of a physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pMemoryProperties - Caller-provided memory-properties structure.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceMemoryProperties)
{
    VKVOID(GetPhysicalDeviceMemoryProperties, H(VkPhysicalDevice, msg->physicalDevice),
           msg->pMemoryProperties);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties

    SYNOPSIS
        struct pHidd_Vulkan_GetPhysicalDeviceQueueFamilyProperties

    LOCATION
        hidd.vulkan

    FUNCTION
        Enumerates the queue families of a physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pQueueFamilyPropertyCount - Pointer to the queue-family count.
        pQueueFamilyProperties - Optional output array of queue-family
            properties.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetPhysicalDeviceQueueFamilyProperties)
{
    VKVOID(GetPhysicalDeviceQueueFamilyProperties, H(VkPhysicalDevice, msg->physicalDevice),
           (uint32_t *)msg->pQueueFamilyPropertyCount, msg->pQueueFamilyProperties);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_EnumerateDeviceExtensionProperties

    SYNOPSIS
        struct pHidd_Vulkan_EnumerateDeviceExtensionProperties

    LOCATION
        hidd.vulkan

    FUNCTION
        Enumerates the extensions exposed by a physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pLayerName - Optional layer name passed to the ICD.
        pPropertyCount - Pointer to the property count.
        pProperties - Caller-provided output properties structure or
            array.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateDeviceExtensionProperties)
{
    VKCALL(EnumerateDeviceExtensionProperties, H(VkPhysicalDevice, msg->physicalDevice),
           msg->pLayerName, (uint32_t *)msg->pPropertyCount, msg->pProperties);
}

/* ==================================================================
 * Device lifecycle
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateDevice

    SYNOPSIS
        struct pHidd_Vulkan_CreateDevice

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a logical Vulkan device for the selected physical device.

    INPUTS
        physicalDevice - Vulkan physical-device handle.
        pCreateInfo - Pointer to the operation's Vulkan create-info
            structure.

    RESULT
        Vulkan object handle, or NULL if creation fails or the entry point is
        absent.

    SEE ALSO
        moHidd_Vulkan_DestroyDevice

*****************************************************************************/

APTR METHOD(HiddVulkan, Hidd_Vulkan, CreateDevice)
{
    VkDevice device = VK_NULL_HANDLE;
    VKFN(CreateDevice);

    D(bug("[Vulkan] %s(physDev=%p)\n", __func__, msg->physicalDevice));
    if (!fn || fn(H(VkPhysicalDevice, msg->physicalDevice), msg->pCreateInfo, NULL, &device) != VK_SUCCESS)
        return NULL;
    return (APTR)device;
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyDevice

    SYNOPSIS
        struct pHidd_Vulkan_DestroyDevice

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a logical Vulkan device.

    INPUTS
        device - Vulkan logical-device handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    SEE ALSO
        moHidd_Vulkan_CreateDevice

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDevice)
{
    VKVOID(DestroyDevice, H(VkDevice, msg->device), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetDeviceQueue

    SYNOPSIS
        struct pHidd_Vulkan_GetDeviceQueue

    LOCATION
        hidd.vulkan

    FUNCTION
        Retrieves a queue handle from a logical device.

    INPUTS
        device - Vulkan logical-device handle.
        queueFamilyIndex - Index of a queue family.
        queueIndex - Index of a queue within that family.
        pQueue - Required output pointer for a queue handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    NOTES
        The base implementation initializes a local queue handle to NULL and
        writes it to pQueue after attempting vkGetDeviceQueue.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetDeviceQueue)
{
    VkQueue queue = VK_NULL_HANDLE;

    VKVOID(GetDeviceQueue, H(VkDevice, msg->device), msg->queueFamilyIndex, msg->queueIndex, &queue);
    OUT(msg->pQueue, queue);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_DeviceWaitIdle

    SYNOPSIS
        struct pHidd_Vulkan_DeviceWaitIdle

    LOCATION
        hidd.vulkan

    FUNCTION
        Waits for outstanding operations on a logical device to finish.

    INPUTS
        device - Vulkan logical-device handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, DeviceWaitIdle)
{
    VKCALL(DeviceWaitIdle, H(VkDevice, msg->device));
}

/* ==================================================================
 * Display: a driver that can show an image overrides this
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_DisplayResource

    SYNOPSIS
        struct pHidd_Vulkan_DisplayResource

    LOCATION
        hidd.vulkan

    FUNCTION
        Provides an optional driver-specific path for presenting an image.

    INPUTS
        image - Driver-owned image or resource handle.
        srcx - Source x coordinate.
        srcy - Source y coordinate.
        rastPort - Destination RastPort.
        dstx - Destination x coordinate.
        dsty - Destination y coordinate.
        width - Width of the requested region.
        height - Height of the requested region.

    RESULT
        No value. The base class performs no presentation.

    NOTES
        The base implementation has an empty method body. A display-capable
        driver must provide the presentation behavior.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DisplayResource)
{
}

/* ==================================================================
 * Memory
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_AllocateMemory

    SYNOPSIS
        struct pHidd_Vulkan_AllocateMemory

    LOCATION
        hidd.vulkan

    FUNCTION
        Allocates device memory of the requested size and memory type.

    INPUTS
        device - Vulkan logical-device handle.
        allocationSize - Number of bytes to allocate.
        memoryTypeIndex - Vulkan memory-type index.
        pMemory - Required output pointer for a device-memory handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

    SEE ALSO
        moHidd_Vulkan_FreeMemory, moHidd_Vulkan_MapMemory

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_FreeMemory

    SYNOPSIS
        struct pHidd_Vulkan_FreeMemory

    LOCATION
        hidd.vulkan

    FUNCTION
        Frees a Vulkan device-memory allocation.

    INPUTS
        device - Vulkan logical-device handle.
        memory - Vulkan device-memory handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    SEE ALSO
        moHidd_Vulkan_AllocateMemory

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeMemory)
{
    VKVOID(FreeMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_MapMemory

    SYNOPSIS
        struct pHidd_Vulkan_MapMemory

    LOCATION
        hidd.vulkan

    FUNCTION
        Maps an allocation into the host address space.

    INPUTS
        device - Vulkan logical-device handle.
        memory - Vulkan device-memory handle.
        offset - Byte offset in the specified memory or command range.
        size - Size of the requested range in bytes.
        flags - Operation-specific Vulkan flag bits.
        ppData - Required output pointer for the mapped host address.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

    SEE ALSO
        moHidd_Vulkan_UnmapMemory

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, MapMemory)
{
    VKCALL(MapMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory),
           msg->offset, msg->size, msg->flags, (void **)msg->ppData);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_UnmapMemory

    SYNOPSIS
        struct pHidd_Vulkan_UnmapMemory

    LOCATION
        hidd.vulkan

    FUNCTION
        Unmaps a device-memory allocation.

    INPUTS
        device - Vulkan logical-device handle.
        memory - Vulkan device-memory handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    SEE ALSO
        moHidd_Vulkan_MapMemory

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, UnmapMemory)
{
    VKVOID(UnmapMemory, H(VkDevice, msg->device), H(VkDeviceMemory, msg->memory));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetMemoryHostPointerPropertiesEXT

    SYNOPSIS
        struct pHidd_Vulkan_GetMemoryHostPointerPropertiesEXT

    LOCATION
        hidd.vulkan

    FUNCTION
        Queries external-memory properties for a host pointer.

    INPUTS
        device - Vulkan logical-device handle.
        handleType - External-memory handle type.
        pHostPointer - Host pointer to query.
        pMemoryHostPointerProperties - Required output properties
            structure.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetMemoryHostPointerPropertiesEXT)
{
    VKCALL(GetMemoryHostPointerPropertiesEXT, H(VkDevice, msg->device), msg->handleType,
           msg->pHostPointer, msg->pMemoryHostPointerProperties);
}

/* ==================================================================
 * Buffers
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateBuffer

    SYNOPSIS
        struct pHidd_Vulkan_CreateBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a buffer with the requested size and usage.

    INPUTS
        device - Vulkan logical-device handle.
        size - Size of the requested range in bytes.
        usage - Vulkan buffer-usage flag bits.
        sharingMode - Buffer-sharing mode.
        pBuffer - Required output pointer for a buffer handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        The base method sets size, usage, and sharingMode in
        VkBufferCreateInfo. It does not supply queueFamilyIndexCount or
        pQueueFamilyIndices, including for concurrent sharing mode.

        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

    SEE ALSO
        moHidd_Vulkan_DestroyBuffer, moHidd_Vulkan_BindBufferMemory

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyBuffer

    SYNOPSIS
        struct pHidd_Vulkan_DestroyBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a Vulkan buffer.

    INPUTS
        device - Vulkan logical-device handle.
        buffer - Vulkan buffer handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

    SEE ALSO
        moHidd_Vulkan_CreateBuffer

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyBuffer)
{
    VKVOID(DestroyBuffer, H(VkDevice, msg->device), H(VkBuffer, msg->buffer), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_BindBufferMemory

    SYNOPSIS
        struct pHidd_Vulkan_BindBufferMemory

    LOCATION
        hidd.vulkan

    FUNCTION
        Binds a device-memory allocation to a buffer.

    INPUTS
        device - Vulkan logical-device handle.
        buffer - Vulkan buffer handle.
        memory - Vulkan device-memory handle.
        memoryOffset - Offset into the bound device-memory allocation.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BindBufferMemory)
{
    VKCALL(BindBufferMemory, H(VkDevice, msg->device), H(VkBuffer, msg->buffer),
           H(VkDeviceMemory, msg->memory), msg->memoryOffset);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetBufferMemoryRequirements

    SYNOPSIS
        struct pHidd_Vulkan_GetBufferMemoryRequirements

    LOCATION
        hidd.vulkan

    FUNCTION
        Queries the memory requirements of a buffer.

    INPUTS
        device - Vulkan logical-device handle.
        buffer - Vulkan buffer handle.
        pMemoryRequirements - Required output memory-requirements
            structure.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, GetBufferMemoryRequirements)
{
    VKVOID(GetBufferMemoryRequirements, H(VkDevice, msg->device), H(VkBuffer, msg->buffer),
           msg->pMemoryRequirements);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetBufferDeviceAddress

    SYNOPSIS
        struct pHidd_Vulkan_GetBufferDeviceAddress

    LOCATION
        hidd.vulkan

    FUNCTION
        Queries a buffer's Vulkan device address.

    INPUTS
        device - Vulkan logical-device handle.
        pInfo - Pointer to the Vulkan query-info structure.

    RESULT
        Device address converted to IPTR, or zero when the entry point is
        unavailable.

    NOTES
        The base method returns IPTR, while vkGetBufferDeviceAddress returns a
        Vulkan device address. On a target where IPTR is narrower than that
        address, the conversion cannot represent all possible values.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateDescriptorSetLayout

    SYNOPSIS
        struct pHidd_Vulkan_CreateDescriptorSetLayout

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a layout describing descriptor-set bindings.

    INPUTS
        device - Vulkan logical-device handle.
        pCreateInfo - Pointer to the operation's Vulkan create-info
            structure.
        pSetLayout - Required output pointer for a descriptor-set layout.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyDescriptorSetLayout

    SYNOPSIS
        struct pHidd_Vulkan_DestroyDescriptorSetLayout

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a descriptor-set layout.

    INPUTS
        device - Vulkan logical-device handle.
        descriptorSetLayout - Descriptor-set layout handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorSetLayout)
{
    VKVOID(DestroyDescriptorSetLayout, H(VkDevice, msg->device),
           H(VkDescriptorSetLayout, msg->descriptorSetLayout), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateDescriptorPool

    SYNOPSIS
        struct pHidd_Vulkan_CreateDescriptorPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a pool used to allocate descriptor sets.

    INPUTS
        device - Vulkan logical-device handle.
        pCreateInfo - Pointer to the operation's Vulkan create-info
            structure.
        pDescriptorPool - Required output pointer for a descriptor-pool
            handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyDescriptorPool

    SYNOPSIS
        struct pHidd_Vulkan_DestroyDescriptorPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a descriptor pool.

    INPUTS
        device - Vulkan logical-device handle.
        descriptorPool - Descriptor-pool handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyDescriptorPool)
{
    VKVOID(DestroyDescriptorPool, H(VkDevice, msg->device), H(VkDescriptorPool, msg->descriptorPool), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_AllocateDescriptorSets

    SYNOPSIS
        struct pHidd_Vulkan_AllocateDescriptorSets

    LOCATION
        hidd.vulkan

    FUNCTION
        Allocates descriptor sets from a descriptor pool.

    INPUTS
        device - Vulkan logical-device handle.
        pAllocateInfo - Vulkan allocation parameters.
        pDescriptorSets - Caller-provided descriptor-set handle array.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, AllocateDescriptorSets)
{
    VKCALL(AllocateDescriptorSets, H(VkDevice, msg->device), msg->pAllocateInfo,
           HA(VkDescriptorSet, msg->pDescriptorSets));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_UpdateDescriptorSets

    SYNOPSIS
        struct pHidd_Vulkan_UpdateDescriptorSets

    LOCATION
        hidd.vulkan

    FUNCTION
        Updates the descriptors referenced by descriptor sets.

    INPUTS
        device - Vulkan logical-device handle.
        descriptorWriteCount - Number of descriptor write operations.
        pDescriptorWrites - Array of descriptor write descriptions.
        descriptorCopyCount - Number of descriptor copy operations.
        pDescriptorCopies - Array of descriptor copy descriptions.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, UpdateDescriptorSets)
{
    VKVOID(UpdateDescriptorSets, H(VkDevice, msg->device), msg->descriptorWriteCount,
           msg->pDescriptorWrites, msg->descriptorCopyCount, msg->pDescriptorCopies);
}

/* ==================================================================
 * Command pools and buffers
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateCommandPool

    SYNOPSIS
        struct pHidd_Vulkan_CreateCommandPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a command pool for a queue family.

    INPUTS
        device - Vulkan logical-device handle.
        queueFamilyIndex - Index of a queue family.
        flags - Operation-specific Vulkan flag bits.
        pCommandPool - Required output pointer for a command-pool handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyCommandPool

    SYNOPSIS
        struct pHidd_Vulkan_DestroyCommandPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a command pool.

    INPUTS
        device - Vulkan logical-device handle.
        commandPool - Vulkan command-pool handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyCommandPool)
{
    VKVOID(DestroyCommandPool, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_ResetCommandPool

    SYNOPSIS
        struct pHidd_Vulkan_ResetCommandPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Resets a command pool and its allocated command buffers.

    INPUTS
        device - Vulkan logical-device handle.
        commandPool - Vulkan command-pool handle.
        flags - Operation-specific Vulkan flag bits.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetCommandPool)
{
    VKCALL(ResetCommandPool, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool), msg->flags);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_AllocateCommandBuffers

    SYNOPSIS
        struct pHidd_Vulkan_AllocateCommandBuffers

    LOCATION
        hidd.vulkan

    FUNCTION
        Allocates command buffers from a command pool.

    INPUTS
        device - Vulkan logical-device handle.
        commandPool - Vulkan command-pool handle.
        level - Command-buffer level.
        count - Number of command buffers.
        pCommandBuffers - Array of command-buffer handles.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_FreeCommandBuffers

    SYNOPSIS
        struct pHidd_Vulkan_FreeCommandBuffers

    LOCATION
        hidd.vulkan

    FUNCTION
        Frees command buffers allocated from a command pool.

    INPUTS
        device - Vulkan logical-device handle.
        commandPool - Vulkan command-pool handle.
        count - Number of command buffers.
        pCommandBuffers - Array of command-buffer handles.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, FreeCommandBuffers)
{
    VKVOID(FreeCommandBuffers, H(VkDevice, msg->device), H(VkCommandPool, msg->commandPool),
           msg->count, HA(const VkCommandBuffer, msg->pCommandBuffers));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_BeginCommandBuffer

    SYNOPSIS
        struct pHidd_Vulkan_BeginCommandBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Begins recording commands in a command buffer.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        flags - Operation-specific Vulkan flag bits.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

    SEE ALSO
        moHidd_Vulkan_EndCommandBuffer

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, BeginCommandBuffer)
{
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = msg->flags,
    };

    VKCALL(BeginCommandBuffer, H(VkCommandBuffer, msg->commandBuffer), &info);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_EndCommandBuffer

    SYNOPSIS
        struct pHidd_Vulkan_EndCommandBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Ends command-buffer recording.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

    SEE ALSO
        moHidd_Vulkan_BeginCommandBuffer

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EndCommandBuffer)
{
    VKCALL(EndCommandBuffer, H(VkCommandBuffer, msg->commandBuffer));
}

/* ==================================================================
 * Queue submission
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_QueueSubmit

    SYNOPSIS
        struct pHidd_Vulkan_QueueSubmit

    LOCATION
        hidd.vulkan

    FUNCTION
        Submits command buffers to a Vulkan queue.

    INPUTS
        queue - Vulkan queue handle.
        submitCount - Number of queue submissions.
        pSubmits - Array of Vulkan submit descriptions.
        fence - Vulkan fence handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueSubmit)
{
    VKCALL(QueueSubmit, H(VkQueue, msg->queue), msg->submitCount, msg->pSubmits, H(VkFence, msg->fence));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_QueueWaitIdle

    SYNOPSIS
        struct pHidd_Vulkan_QueueWaitIdle

    LOCATION
        hidd.vulkan

    FUNCTION
        Waits until a Vulkan queue is idle.

    INPUTS
        queue - Vulkan queue handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, QueueWaitIdle)
{
    VKCALL(QueueWaitIdle, H(VkQueue, msg->queue));
}

/* ==================================================================
 * Fences
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateFence

    SYNOPSIS
        struct pHidd_Vulkan_CreateFence

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a fence used for synchronization.

    INPUTS
        device - Vulkan logical-device handle.
        flags - Operation-specific Vulkan flag bits.
        pFence - Required output pointer for a fence handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyFence

    SYNOPSIS
        struct pHidd_Vulkan_DestroyFence

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a fence.

    INPUTS
        device - Vulkan logical-device handle.
        fence - Vulkan fence handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyFence)
{
    VKVOID(DestroyFence, H(VkDevice, msg->device), H(VkFence, msg->fence), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_WaitForFences

    SYNOPSIS
        struct pHidd_Vulkan_WaitForFences

    LOCATION
        hidd.vulkan

    FUNCTION
        Waits for the specified fences.

    INPUTS
        device - Vulkan logical-device handle.
        fenceCount - Number of fence handles.
        pFences - Array of Vulkan fence handles.
        waitAll - Whether all fences must be signaled.
        timeout - Timeout in nanoseconds.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, WaitForFences)
{
    VKCALL(WaitForFences, H(VkDevice, msg->device), msg->fenceCount, HA(const VkFence, msg->pFences),
           msg->waitAll, msg->timeout);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_ResetFences

    SYNOPSIS
        struct pHidd_Vulkan_ResetFences

    LOCATION
        hidd.vulkan

    FUNCTION
        Resets one or more fences.

    INPUTS
        device - Vulkan logical-device handle.
        fenceCount - Number of fence handles.
        pFences - Array of Vulkan fence handles.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetFences)
{
    VKCALL(ResetFences, H(VkDevice, msg->device), msg->fenceCount, HA(const VkFence, msg->pFences));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetFenceStatus

    SYNOPSIS
        struct pHidd_Vulkan_GetFenceStatus

    LOCATION
        hidd.vulkan

    FUNCTION
        Queries whether a fence is signaled.

    INPUTS
        device - Vulkan logical-device handle.
        fence - Vulkan fence handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetFenceStatus)
{
    VKCALL(GetFenceStatus, H(VkDevice, msg->device), H(VkFence, msg->fence));
}

/* ==================================================================
 * Semaphores
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateSemaphore

    SYNOPSIS
        struct pHidd_Vulkan_CreateSemaphore

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a semaphore used for queue synchronization.

    INPUTS
        device - Vulkan logical-device handle.
        pCreateInfo - Pointer to the operation's Vulkan create-info
            structure.
        pSemaphore - Required output pointer for a semaphore handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If pCreateInfo is NULL, the base implementation supplies a
        VkSemaphoreCreateInfo containing sType and no explicit flags.

        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroySemaphore

    SYNOPSIS
        struct pHidd_Vulkan_DestroySemaphore

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a semaphore.

    INPUTS
        device - Vulkan logical-device handle.
        semaphore - Vulkan semaphore handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroySemaphore)
{
    VKVOID(DestroySemaphore, H(VkDevice, msg->device), H(VkSemaphore, msg->semaphore), NULL);
}

/* ==================================================================
 * Events
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateEvent

    SYNOPSIS
        struct pHidd_Vulkan_CreateEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a Vulkan event.

    INPUTS
        device - Vulkan logical-device handle.
        pEvent - Required output pointer for an event handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        The base implementation passes a VkEventCreateInfo with sType set to
        VK_STRUCTURE_TYPE_EVENT_CREATE_INFO.

        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyEvent

    SYNOPSIS
        struct pHidd_Vulkan_DestroyEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a Vulkan event.

    INPUTS
        device - Vulkan logical-device handle.
        event - Vulkan event handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyEvent)
{
    VKVOID(DestroyEvent, H(VkDevice, msg->device), H(VkEvent, msg->event), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_SetEvent

    SYNOPSIS
        struct pHidd_Vulkan_SetEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Sets the signaled state of an event.

    INPUTS
        device - Vulkan logical-device handle.
        event - Vulkan event handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, SetEvent)
{
    VKCALL(SetEvent, H(VkDevice, msg->device), H(VkEvent, msg->event));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_ResetEvent

    SYNOPSIS
        struct pHidd_Vulkan_ResetEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Clears the signaled state of an event.

    INPUTS
        device - Vulkan logical-device handle.
        event - Vulkan event handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, ResetEvent)
{
    VKCALL(ResetEvent, H(VkDevice, msg->device), H(VkEvent, msg->event));
}

/* ==================================================================
 * Shaders and pipelines
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateShaderModule

    SYNOPSIS
        struct pHidd_Vulkan_CreateShaderModule

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a shader module from SPIR-V code.

    INPUTS
        device - Vulkan logical-device handle.
        pCode - Pointer to SPIR-V code words.
        codeSize - SPIR-V code size in bytes.
        pShaderModule - Required output pointer for a shader-module
            handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        The method constructs VkShaderModuleCreateInfo with the supplied code
        and codeSize before invoking the ICD.

        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyShaderModule

    SYNOPSIS
        struct pHidd_Vulkan_DestroyShaderModule

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a shader module.

    INPUTS
        device - Vulkan logical-device handle.
        shaderModule - Vulkan shader-module handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyShaderModule)
{
    VKVOID(DestroyShaderModule, H(VkDevice, msg->device), H(VkShaderModule, msg->shaderModule), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreatePipelineLayout

    SYNOPSIS
        struct pHidd_Vulkan_CreatePipelineLayout

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a layout for descriptor sets and push constants.

    INPUTS
        device - Vulkan logical-device handle.
        setLayoutCount - Number of descriptor-set layouts.
        pSetLayouts - Array of descriptor-set layouts.
        pushConstantRangeCount - Number of push-constant ranges.
        pPushConstantRanges - Array of push-constant ranges.
        pPipelineLayout - Required output pointer for a pipeline-layout
            handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyPipelineLayout

    SYNOPSIS
        struct pHidd_Vulkan_DestroyPipelineLayout

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a pipeline layout.

    INPUTS
        device - Vulkan logical-device handle.
        pipelineLayout - Vulkan pipeline-layout handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipelineLayout)
{
    VKVOID(DestroyPipelineLayout, H(VkDevice, msg->device), H(VkPipelineLayout, msg->pipelineLayout), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateComputePipelines

    SYNOPSIS
        struct pHidd_Vulkan_CreateComputePipelines

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates compute pipelines from Vulkan create-info structures.

    INPUTS
        device - Vulkan logical-device handle.
        createInfoCount - Number of pipeline create-info structures.
        pCreateInfos - Array of pipeline create-info structures.
        pPipelines - Caller-provided output array of pipeline handles.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

    NOTES
        The base implementation passes VK_NULL_HANDLE as the pipeline cache.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, CreateComputePipelines)
{
    VKCALL(CreateComputePipelines, H(VkDevice, msg->device), VK_NULL_HANDLE, msg->createInfoCount,
           msg->pCreateInfos, NULL, HA(VkPipeline, msg->pPipelines));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyPipeline

    SYNOPSIS
        struct pHidd_Vulkan_DestroyPipeline

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a Vulkan pipeline.

    INPUTS
        device - Vulkan logical-device handle.
        pipeline - Vulkan pipeline handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyPipeline)
{
    VKVOID(DestroyPipeline, H(VkDevice, msg->device), H(VkPipeline, msg->pipeline), NULL);
}

/* ==================================================================
 * Query pools
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CreateQueryPool

    SYNOPSIS
        struct pHidd_Vulkan_CreateQueryPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Creates a query pool.

    INPUTS
        device - Vulkan logical-device handle.
        pCreateInfo - Pointer to the operation's Vulkan create-info
            structure.
        pQueryPool - Required output pointer for a query-pool handle.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED when the
        entry point is absent.

    NOTES
        If the ICD entry point is unavailable, the base method returns
        VK_ERROR_INITIALIZATION_FAILED without writing its output handle.
        When the ICD entry point is called, the returned handle is written
        even if the ICD reports a failure; the local handle starts as NULL.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_DestroyQueryPool

    SYNOPSIS
        struct pHidd_Vulkan_DestroyQueryPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Destroys a query pool.

    INPUTS
        device - Vulkan logical-device handle.
        queryPool - Vulkan query-pool handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, DestroyQueryPool)
{
    VKVOID(DestroyQueryPool, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool), NULL);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetQueryPoolResults

    SYNOPSIS
        struct pHidd_Vulkan_GetQueryPoolResults

    LOCATION
        hidd.vulkan

    FUNCTION
        Retrieves data from queries in a query pool.

    INPUTS
        device - Vulkan logical-device handle.
        queryPool - Vulkan query-pool handle.
        firstQuery - Index of the first query.
        queryCount - Number of queries.
        dataSize - Size of the destination buffer in bytes.
        pData - Destination buffer for query results.
        stride - Byte stride between query results.
        flags - Operation-specific Vulkan flag bits.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, GetQueryPoolResults)
{
    VKCALL(GetQueryPoolResults, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount, msg->dataSize, msg->pData, msg->stride, msg->flags);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_ResetQueryPool

    SYNOPSIS
        struct pHidd_Vulkan_ResetQueryPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Resets a range of queries.

    INPUTS
        device - Vulkan logical-device handle.
        queryPool - Vulkan query-pool handle.
        firstQuery - Index of the first query.
        queryCount - Number of queries.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, ResetQueryPool)
{
    VKVOID(ResetQueryPool, H(VkDevice, msg->device), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount);
}

/* ==================================================================
 * Command recording
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdBindPipeline

    SYNOPSIS
        struct pHidd_Vulkan_CmdBindPipeline

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a pipeline-binding command.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        pipelineBindPoint - Vulkan pipeline bind point.
        pipeline - Vulkan pipeline handle.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindPipeline)
{
    VKVOID(CmdBindPipeline, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineBindPoint,
           H(VkPipeline, msg->pipeline));
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdBindDescriptorSets

    SYNOPSIS
        struct pHidd_Vulkan_CmdBindDescriptorSets

    LOCATION
        hidd.vulkan

    FUNCTION
        Records descriptor-set bindings.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        pipelineBindPoint - Vulkan pipeline bind point.
        layout - Vulkan pipeline-layout handle.
        firstSet - Index of the first descriptor set.
        descriptorSetCount - Number of descriptor sets.
        pDescriptorSets - Caller-provided descriptor-set handle array.
        dynamicOffsetCount - Number of dynamic offsets.
        pDynamicOffsets - Array of dynamic offsets.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdBindDescriptorSets)
{
    VKVOID(CmdBindDescriptorSets, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineBindPoint,
           H(VkPipelineLayout, msg->layout), msg->firstSet, msg->descriptorSetCount,
           HA(const VkDescriptorSet, msg->pDescriptorSets), msg->dynamicOffsetCount,
           (const uint32_t *)msg->pDynamicOffsets);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdPushConstants

    SYNOPSIS
        struct pHidd_Vulkan_CmdPushConstants

    LOCATION
        hidd.vulkan

    FUNCTION
        Records an update to push-constant values.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        layout - Vulkan pipeline-layout handle.
        stageFlags - Shader-stage selection flags.
        offset - Byte offset in the specified memory or command range.
        size - Size of the requested range in bytes.
        pValues - Push-constant data.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPushConstants)
{
    VKVOID(CmdPushConstants, H(VkCommandBuffer, msg->commandBuffer), H(VkPipelineLayout, msg->layout),
           msg->stageFlags, msg->offset, msg->size, msg->pValues);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdDispatch

    SYNOPSIS
        struct pHidd_Vulkan_CmdDispatch

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a compute-workgroup dispatch.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        groupCountX - Number of workgroups along the x axis.
        groupCountY - Number of workgroups along the y axis.
        groupCountZ - Number of workgroups along the z axis.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdDispatch)
{
    VKVOID(CmdDispatch, H(VkCommandBuffer, msg->commandBuffer), msg->groupCountX, msg->groupCountY,
           msg->groupCountZ);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdCopyBuffer

    SYNOPSIS
        struct pHidd_Vulkan_CmdCopyBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a buffer-to-buffer copy.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        srcBuffer - Source Vulkan buffer.
        dstBuffer - Destination Vulkan buffer.
        regionCount - Number of copy regions.
        pRegions - Array of Vulkan buffer-copy regions.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdCopyBuffer)
{
    VKVOID(CmdCopyBuffer, H(VkCommandBuffer, msg->commandBuffer), H(VkBuffer, msg->srcBuffer),
           H(VkBuffer, msg->dstBuffer), msg->regionCount, msg->pRegions);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdFillBuffer

    SYNOPSIS
        struct pHidd_Vulkan_CmdFillBuffer

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a buffer-fill operation.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        dstBuffer - Destination Vulkan buffer.
        dstOffset - Destination offset in bytes.
        size - Size of the requested range in bytes.
        data - 32-bit fill value.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdFillBuffer)
{
    VKVOID(CmdFillBuffer, H(VkCommandBuffer, msg->commandBuffer), H(VkBuffer, msg->dstBuffer),
           msg->dstOffset, msg->size, msg->data);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdPipelineBarrier

    SYNOPSIS
        struct pHidd_Vulkan_CmdPipelineBarrier

    LOCATION
        hidd.vulkan

    FUNCTION
        Records execution and memory dependencies.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        srcStageMask - Source pipeline-stage mask.
        dstStageMask - Destination pipeline-stage mask.
        dependencyFlags - Vulkan dependency flags.
        memoryBarrierCount - Number of memory barriers.
        pMemoryBarriers - Array of memory barriers.
        bufferMemoryBarrierCount - Number of buffer-memory barriers.
        pBufferMemoryBarriers - Array of buffer-memory barriers.
        imageMemoryBarrierCount - Number of image-memory barriers.
        pImageMemoryBarriers - Array of image-memory barriers.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdPipelineBarrier)
{
    VKVOID(CmdPipelineBarrier, H(VkCommandBuffer, msg->commandBuffer), msg->srcStageMask, msg->dstStageMask,
           msg->dependencyFlags, msg->memoryBarrierCount, msg->pMemoryBarriers,
           msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
           msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdSetEvent

    SYNOPSIS
        struct pHidd_Vulkan_CmdSetEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a command that sets an event.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        event - Vulkan event handle.
        stageMask - Pipeline-stage mask.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdSetEvent)
{
    VKVOID(CmdSetEvent, H(VkCommandBuffer, msg->commandBuffer), H(VkEvent, msg->event), msg->stageMask);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdResetEvent

    SYNOPSIS
        struct pHidd_Vulkan_CmdResetEvent

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a command that resets an event.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        event - Vulkan event handle.
        stageMask - Pipeline-stage mask.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetEvent)
{
    VKVOID(CmdResetEvent, H(VkCommandBuffer, msg->commandBuffer), H(VkEvent, msg->event), msg->stageMask);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdWaitEvents

    SYNOPSIS
        struct pHidd_Vulkan_CmdWaitEvents

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a command that waits for events.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        eventCount - Number of events.
        pEvents - Array of event handles.
        srcStageMask - Source pipeline-stage mask.
        dstStageMask - Destination pipeline-stage mask.
        memoryBarrierCount - Number of memory barriers.
        pMemoryBarriers - Array of memory barriers.
        bufferMemoryBarrierCount - Number of buffer-memory barriers.
        pBufferMemoryBarriers - Array of buffer-memory barriers.
        imageMemoryBarrierCount - Number of image-memory barriers.
        pImageMemoryBarriers - Array of image-memory barriers.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWaitEvents)
{
    VKVOID(CmdWaitEvents, H(VkCommandBuffer, msg->commandBuffer), msg->eventCount,
           HA(const VkEvent, msg->pEvents), msg->srcStageMask, msg->dstStageMask,
           msg->memoryBarrierCount, msg->pMemoryBarriers,
           msg->bufferMemoryBarrierCount, msg->pBufferMemoryBarriers,
           msg->imageMemoryBarrierCount, msg->pImageMemoryBarriers);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdWriteTimestamp

    SYNOPSIS
        struct pHidd_Vulkan_CmdWriteTimestamp

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a timestamp write to a query pool.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        pipelineStage - Pipeline stage for the timestamp.
        queryPool - Vulkan query-pool handle.
        query - Index of the destination query.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdWriteTimestamp)
{
    VKVOID(CmdWriteTimestamp, H(VkCommandBuffer, msg->commandBuffer), msg->pipelineStage,
           H(VkQueryPool, msg->queryPool), msg->query);
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_CmdResetQueryPool

    SYNOPSIS
        struct pHidd_Vulkan_CmdResetQueryPool

    LOCATION
        hidd.vulkan

    FUNCTION
        Records a command that resets queries.

    INPUTS
        commandBuffer - Vulkan command-buffer handle.
        queryPool - Vulkan query-pool handle.
        firstQuery - Index of the first query.
        queryCount - Number of queries.

    RESULT
        No return value. If the Vulkan entry point is absent, the base method
        does not invoke it.

*****************************************************************************/

VOID METHOD(HiddVulkan, Hidd_Vulkan, CmdResetQueryPool)
{
    VKVOID(CmdResetQueryPool, H(VkCommandBuffer, msg->commandBuffer), H(VkQueryPool, msg->queryPool),
           msg->firstQuery, msg->queryCount);
}

/* ==================================================================
 * ICD-style function lookup: GetInstanceProcAddr is the driver's
 * ================================================================== */

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetInstanceProcAddr

    SYNOPSIS
        struct pHidd_Vulkan_GetInstanceProcAddr

    LOCATION
        hidd.vulkan

    FUNCTION
        Resolves an instance-level Vulkan function by name.

    INPUTS
        instance - Vulkan instance handle.
        pName - NUL-terminated Vulkan entry-point name.

    RESULT
        NULL from the base class. A subclass can return an ICD function
        pointer.

    NOTES
        The base implementation always returns NULL. Driver subclasses must
        override this method to supply their Vulkan ICD entry points.

    SEE ALSO
        moHidd_Vulkan_GetDeviceProcAddr

*****************************************************************************/

APTR METHOD(HiddVulkan, Hidd_Vulkan, GetInstanceProcAddr)
{
    return NULL;
}

/*****************************************************************************
    NAME
        moHidd_Vulkan_GetDeviceProcAddr

    SYNOPSIS
        struct pHidd_Vulkan_GetDeviceProcAddr

    LOCATION
        hidd.vulkan

    FUNCTION
        Resolves a device-level Vulkan function by name.

    INPUTS
        device - Vulkan logical-device handle.
        pName - NUL-terminated Vulkan entry-point name.

    RESULT
        ICD function pointer, or NULL if the required entry point is
        unavailable.

    NOTES
        The base implementation looks up vkGetDeviceProcAddr through the
        instance dispatch path retained by CreateInstance. A missing entry
        point returns NULL.

    SEE ALSO
        moHidd_Vulkan_GetInstanceProcAddr

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_EnumerateInstanceVersion

    SYNOPSIS
        struct pHidd_Vulkan_EnumerateInstanceVersion

    LOCATION
        hidd.vulkan

    FUNCTION
        Reads the Vulkan API version exposed by the ICD.

    INPUTS
        pApiVersion - Required output pointer for the Vulkan API version.

    RESULT
        VkResult converted to IPTR. If the entry point is absent, the method
        writes Vulkan 1.0 and returns VK_SUCCESS.

    NOTES
        If the ICD lacks vkEnumerateInstanceVersion, the base implementation
        writes VK_API_VERSION_1_0 and returns VK_SUCCESS.

*****************************************************************************/

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

/*****************************************************************************
    NAME
        moHidd_Vulkan_EnumerateInstanceExtensionProperties

    SYNOPSIS
        struct pHidd_Vulkan_EnumerateInstanceExtensionProperties

    LOCATION
        hidd.vulkan

    FUNCTION
        Enumerates the ICD's instance extensions.

    INPUTS
        pLayerName - Optional layer name passed to the ICD.
        pPropertyCount - Pointer to the property count.
        pProperties - Caller-provided output properties structure or
            array.

    RESULT
        VkResult converted to IPTR, or VK_ERROR_INITIALIZATION_FAILED if the
        requested Vulkan entry point is unavailable.

*****************************************************************************/

IPTR METHOD(HiddVulkan, Hidd_Vulkan, EnumerateInstanceExtensionProperties)
{
    VKCALL(EnumerateInstanceExtensionProperties, msg->pLayerName, (uint32_t *)msg->pPropertyCount,
           msg->pProperties);
}
