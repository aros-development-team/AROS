/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    nvk.nvdd: the nvgfx driver's Vulkan part - Mesa's NVK - as a library
    the driver opens when its display is first asked for a Vulkan object.

    The class is a subclass of the vulkan.hidd base class nvgfx passes in
    (taken from the caller's CreateObject); this library never opens
    vulkan.hidd. NVK reaches the kernel driver through the DRM entry table
    in struct nvdd_host, never by linking it.

    New() fails for a card NVK cannot drive - which is how CreateObject
    learns the card has no Vulkan - since the object is only made once
    NVK's own physical device probe (channel, VM_INIT, chipset and
    conformance checks) has succeeded. The base class serves every Vulkan
    method through the two proc-address methods overridden here.
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/vulkan_hidd.h>
#include <nvdd/nvdd.h>

#include "vulkan/vulkan_core.h"
#include "nvk_drm.h"

#include <string.h>

#include LC_LIBDEFS_FILE

OOP_AttrBase HiddVulkanAttrBase;

/* the driver's services, the class, and NVK's answer for the card */
static struct nvdd_host nvk_host;
static OOP_Class *nvk_class;
static LONG nvk_probe_state;        /* 0 not tried, 1 NVK drives the card, -1 not */
static LONG nvk_objects;
static struct SignalSemaphore nvk_lock;

#define nvk_log(...) do { if (nvk_host.log) nvk_host.log(__VA_ARGS__); } while (0)

/* NVK's ICD entry point, and the runtime's device-level lookup it
   dispatches vkGetDeviceProcAddr to */
PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName);
PFN_vkVoidFunction vk_common_GetDeviceProcAddr(VkDevice device, const char *pName);

/* Create an instance and count its physical devices: NVK accepts the
   card, or there is nothing for this class to serve. */
static BOOL nvk_probe(void)
{
    PFN_vkCreateInstance createInstance;
    PFN_vkEnumeratePhysicalDevices enumerate;
    PFN_vkDestroyInstance destroyInstance;
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "nvk.nvdd",
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t count = 0;
    VkResult res;

    createInstance = (PFN_vkCreateInstance)vk_icdGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (!createInstance)
        return FALSE;

    res = createInstance(&createInfo, NULL, &instance);
    if (res != VK_SUCCESS)
    {
        nvk_log("[NVK] vkCreateInstance failed (%d)\n", (int)res);
        return FALSE;
    }

    enumerate = (PFN_vkEnumeratePhysicalDevices)vk_icdGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
    destroyInstance = (PFN_vkDestroyInstance)vk_icdGetInstanceProcAddr(instance, "vkDestroyInstance");

    if (enumerate)
    {
        res = enumerate(instance, &count, NULL);
        if (res != VK_SUCCESS)
            count = 0;
    }

    if (destroyInstance)
        destroyInstance(instance, NULL);

    return count > 0;
}

/* ---- the class ---- */

static OOP_Object *NVKVulkan__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    IPTR interfaceVers;
    OOP_Object *obj;

    interfaceVers = GetTagData(aHidd_Vulkan_InterfaceVersion, -1, msg->attrList);
    if (interfaceVers != VULKAN_INTERFACE_VERSION)
        return NULL;

    /* the probe's answer holds for the card: ask NVK once */
    ObtainSemaphore(&nvk_lock);
    if (nvk_probe_state == 0)
    {
        nvk_probe_state = nvk_probe() ? 1 : -1;
        nvk_log("[NVK] %s\n", (nvk_probe_state > 0) ?
                "NVK drives the card" : "NVK cannot drive the card");
    }
    ReleaseSemaphore(&nvk_lock);
    if (nvk_probe_state < 0)
        return NULL;

    obj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (obj)
        __sync_fetch_and_add(&nvk_objects, 1);
    return obj;
}

static VOID NVKVulkan__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    __sync_fetch_and_sub(&nvk_objects, 1);
    OOP_DoSuperMethod(cl, o, msg);
}

/* ---- ICD-style function lookup ---- */

static APTR NVKVulkan__Hidd_Vulkan__GetInstanceProcAddr(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Vulkan_GetInstanceProcAddr *msg)
{
    if (msg->pName == NULL)
        return NULL;

    if (msg->instance == NULL && strcmp(msg->pName, "vkGetInstanceProcAddr") == 0)
        return (APTR)vk_icdGetInstanceProcAddr;

    return (APTR)vk_icdGetInstanceProcAddr((VkInstance)msg->instance, msg->pName);
}

static APTR NVKVulkan__Hidd_Vulkan__GetDeviceProcAddr(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Vulkan_GetDeviceProcAddr *msg)
{
    if (msg->device == NULL || msg->pName == NULL)
        return NULL;

    return (APTR)vk_common_GetDeviceProcAddr((VkDevice)msg->device, msg->pName);
}

/* ---- library interface (include/nvdd/nvdd.h) ---- */

AROS_LH2(OOP_Class *, NVDDInitClass,
    AROS_LHA(OOP_Class *, base, A0),
    AROS_LHA(const struct nvdd_host *, host, A1),
    LIBBASETYPEPTR, LIBBASE, 5, NVK)
{
    AROS_LIBFUNC_INIT

    OOP_Class *cl;

    if (!base || !host || host->version != NVDD_INTERFACE_VERSION || !host->drm)
        return NULL;

    ObtainSemaphore(&nvk_lock);
    if (!nvk_class)
    {
        struct OOP_MethodDescr root_descr[] =
        {
            {(OOP_MethodFunc)NVKVulkan__Root__New, moRoot_New},
            {(OOP_MethodFunc)NVKVulkan__Root__Dispose, moRoot_Dispose},
            {NULL, 0}
        };
        struct OOP_MethodDescr vulkan_descr[] =
        {
            {(OOP_MethodFunc)NVKVulkan__Hidd_Vulkan__GetInstanceProcAddr, moHidd_Vulkan_GetInstanceProcAddr},
            {(OOP_MethodFunc)NVKVulkan__Hidd_Vulkan__GetDeviceProcAddr, moHidd_Vulkan_GetDeviceProcAddr},
            {NULL, 0}
        };
        struct OOP_InterfaceDescr ifdescr[] =
        {
            {root_descr, IID_Root, 2},
            {vulkan_descr, IID_Hidd_Vulkan, 2},
            {NULL, NULL, 0}
        };
        OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        struct TagItem tags[] =
        {
            {aMeta_SuperPtr, (IPTR)base},
            {aMeta_InterfaceDescr, (IPTR)ifdescr},
            {aMeta_InstSize, sizeof(ULONG)},
            {TAG_DONE, 0}
        };

        /* the driver's services: copy what this library knows of */
        memset(&nvk_host, 0, sizeof(nvk_host));
        CopyMem((APTR)host, &nvk_host, (host->size < sizeof(nvk_host)) ? host->size : sizeof(nvk_host));
        nvk_host.size = sizeof(nvk_host);

        if (!HiddVulkanAttrBase)
            HiddVulkanAttrBase = OOP_ObtainAttrBase(IID_Hidd_Vulkan);
        if (HiddVulkanAttrBase)
        {
            nvk_drm_set_entry(nvk_host.drm);
            nvk_class = (OOP_Class *)OOP_NewObject(NULL, CLID_HiddMeta, tags);
            if (nvk_class)
                nvk_class->UserData = LIBBASE;
        }
    }
    cl = nvk_class;
    ReleaseSemaphore(&nvk_lock);

    return cl;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(OOP_Object *, NVDDNewObject,
    AROS_LHA(OOP_Object *, display, A0),
    AROS_LHA(struct TagItem *, attrList, A1),
    LIBBASETYPEPTR, LIBBASE, 6, NVK)
{
    AROS_LIBFUNC_INIT

    if (!nvk_class)
        return NULL;
    return OOP_NewObject(nvk_class, NULL, attrList);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BOOL, NVDDFreeClass,
    LIBBASETYPEPTR, LIBBASE, 7, NVK)
{
    AROS_LIBFUNC_INIT

    BOOL freed = FALSE;

    ObtainSemaphore(&nvk_lock);
    if (nvk_objects == 0)
    {
        if (nvk_class)
            OOP_DisposeObject((OOP_Object *)nvk_class);
        nvk_class = NULL;
        if (HiddVulkanAttrBase)
            OOP_ReleaseAttrBase(IID_Hidd_Vulkan);
        HiddVulkanAttrBase = 0;
        freed = TRUE;
    }
    ReleaseSemaphore(&nvk_lock);

    return freed;

    AROS_LIBFUNC_EXIT
}

static int NVK_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&nvk_lock);
    return TRUE;
}

ADD2INITLIB(NVK_Init, 0)
