/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vulkan.library: the global commands, the instance and its physical
    devices, and vkGetInstanceProcAddr.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <string.h>

#include "vulkan_intern.h"

/* ---- the command table ---- */

const struct vkl_command *vkl_find_command(const char *name)
{
    LONG lo = 0, hi = VKL_NUM_COMMANDS - 1;

    while (lo <= hi)
    {
        LONG mid = (lo + hi) / 2;
        int c = strcmp(name, vkl_commands[mid].name);

        if (c == 0)
            return &vkl_commands[mid];
        if (c < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return NULL;
}

/* ---- extensions ---- */

/*
 * Window system integration is the library's: whatever the drivers offer
 * of it themselves (Mesa's own WSI, headless) is neither listed nor passed
 * down.
 */
BOOL vkl_ext_hidden_instance(const char *name)
{
    return strstr(name, "surface") || strstr(name, "display") || strstr(name, "swapchain");
}

BOOL vkl_ext_hidden_device(const char *name)
{
    return strstr(name, "swapchain") || strstr(name, "display") || strstr(name, "present") ||
           !strcmp(name, "VK_EXT_hdr_metadata") || !strcmp(name, "VK_EXT_full_screen_exclusive");
}

static const VkExtensionProperties vkl_instance_exts[] =
{
    { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_SPEC_VERSION },
    { VK_AROS_SURFACE_EXTENSION_NAME, VK_AROS_SURFACE_SPEC_VERSION },
    { VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, VK_KHR_GET_SURFACE_CAPABILITIES_2_SPEC_VERSION },
};
#define VKL_NUM_INSTANCE_EXTS (sizeof(vkl_instance_exts) / sizeof(vkl_instance_exts[0]))

static BOOL vkl_is_own_instance_ext(const char *name)
{
    ULONG i;

    for (i = 0; i < VKL_NUM_INSTANCE_EXTS; i++)
        if (!strcmp(name, vkl_instance_exts[i].extensionName))
            return TRUE;
    return FALSE;
}

/* a driver's instance extensions, without the hidden ones; the caller frees */
static VkExtensionProperties *vkl_icd_instance_exts(struct vkl_icd *icd, uint32_t *countp)
{
    PFN_vkEnumerateInstanceExtensionProperties enumerate =
        (PFN_vkEnumerateInstanceExtensionProperties)icd->gipa(NULL, "vkEnumerateInstanceExtensionProperties");
    VkExtensionProperties *props;
    uint32_t count = 0, i, n = 0;

    *countp = 0;
    if (!enumerate || enumerate(NULL, &count, NULL) != VK_SUCCESS || !count)
        return NULL;
    props = vkl_alloc(count * sizeof(*props));
    if (!props)
        return NULL;
    if (enumerate(NULL, &count, props) < 0)
    {
        vkl_free(props);
        return NULL;
    }
    for (i = 0; i < count; i++)
        if (!vkl_ext_hidden_instance(props[i].extensionName))
            props[n++] = props[i];
    *countp = n;
    return props;
}

static BOOL vkl_has_ext(const VkExtensionProperties *props, uint32_t count, const char *name)
{
    uint32_t i;

    for (i = 0; i < count; i++)
        if (!strcmp(props[i].extensionName, name))
            return TRUE;
    return FALSE;
}

/* ---- global commands ---- */

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(uint32_t *pApiVersion)
{
    uint32_t best = VK_API_VERSION_1_0;
    ULONG i;

    vkl_icds_init();
    for (i = 0; i < vkl_icd_count; i++)
    {
        PFN_vkEnumerateInstanceVersion fn =
            (PFN_vkEnumerateInstanceVersion)vkl_icds[i].gipa(NULL, "vkEnumerateInstanceVersion");
        uint32_t v = VK_API_VERSION_1_0;

        if (fn && fn(&v) == VK_SUCCESS && v > best)
            best = v;
    }
    *pApiVersion = best;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
    VkLayerProperties *pProperties)
{
    if (pPropertyCount)
        *pPropertyCount = 0;
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName,
    uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    VkExtensionProperties *all;
    uint32_t total = 0, cap, i, j;
    VkResult res = VK_SUCCESS;
    ULONG n;

    if (pLayerName)
        return VK_ERROR_LAYER_NOT_PRESENT;

    vkl_icds_init();

    cap = VKL_NUM_INSTANCE_EXTS + vkl_icd_count * 128;
    all = vkl_alloc(cap * sizeof(*all));
    if (!all)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    /* the library's own need a driver to draw with */
    if (vkl_icd_count)
        for (i = 0; i < VKL_NUM_INSTANCE_EXTS; i++)
            all[total++] = vkl_instance_exts[i];

    for (n = 0; n < vkl_icd_count; n++)
    {
        uint32_t count;
        VkExtensionProperties *props = vkl_icd_instance_exts(&vkl_icds[n], &count);

        for (j = 0; props && j < count && total < cap; j++)
            if (!vkl_has_ext(all, total, props[j].extensionName))
                all[total++] = props[j];
        vkl_free(props);
    }

    if (!pProperties)
        *pPropertyCount = total;
    else
    {
        if (*pPropertyCount < total)
            res = VK_INCOMPLETE;
        else
            *pPropertyCount = total;
        memcpy(pProperties, all, *pPropertyCount * sizeof(*all));
    }

    vkl_free(all);
    return res;
}

/* ---- instance ---- */

static void vkl_icd_instance_fill(struct vkl_icd_instance *ii)
{
    ULONG i;

    for (i = 0; i < VKL_NUM_COMMANDS; i++)
    {
        const struct vkl_command *cmd = &vkl_commands[i];

        if (cmd->level == VKL_LEVEL_INSTANCE || cmd->level == VKL_LEVEL_PHYSDEV)
            VKL_SLOT(&ii->disp, cmd) = ii->icd->gipa(ii->handle, cmd->name);
    }
    ii->gdpa = (PFN_vkGetDeviceProcAddr)ii->icd->gipa(ii->handle, "vkGetDeviceProcAddr");

    vkl_device_install(ii);
    vkl_wsi_install_physdev(ii);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkInstance *pInstance)
{
    struct vkl_instance *inst;
    VkResult res = VK_ERROR_INCOMPATIBLE_DRIVER;
    const char **names = NULL;
    uint32_t i;
    ULONG n;

    if (!vkl_icds_init())
        return VK_ERROR_INCOMPATIBLE_DRIVER;

    if (pCreateInfo->enabledLayerCount)
        return VK_ERROR_LAYER_NOT_PRESENT;

    inst = vkl_alloc(sizeof(*inst));
    if (!inst)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    inst->magic = VKL_INSTANCE_MAGIC;
    inst->loader = &inst->icd[0].disp;
    InitSemaphore(&inst->lock);
    NEWLIST((struct List *)&inst->surfaces);

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++)
    {
        const char *name = pCreateInfo->ppEnabledExtensionNames[i];

        if (!strcmp(name, VK_KHR_SURFACE_EXTENSION_NAME))
            inst->khrSurface = TRUE;
        else if (!strcmp(name, VK_AROS_SURFACE_EXTENSION_NAME))
            inst->arosSurface = TRUE;
        else if (!vkl_is_own_instance_ext(name) && vkl_ext_hidden_instance(name))
        {
            vkl_free(inst);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    if (pCreateInfo->enabledExtensionCount)
    {
        names = vkl_alloc(pCreateInfo->enabledExtensionCount * sizeof(*names));
        if (!names)
        {
            vkl_free(inst);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }

    /* an instance of each driver, with the extensions it has */
    for (n = 0; n < vkl_icd_count; n++)
    {
        struct vkl_icd *icd = &vkl_icds[n];
        struct vkl_icd_instance *ii = &inst->icd[inst->icdCount];
        PFN_vkCreateInstance create = (PFN_vkCreateInstance)icd->gipa(NULL, "vkCreateInstance");
        VkInstanceCreateInfo info = *pCreateInfo;
        VkExtensionProperties *props;
        uint32_t count, enabled = 0;
        BOOL missing = FALSE;
        VkResult r;

        if (!create)
            continue;

        props = vkl_icd_instance_exts(icd, &count);
        for (i = 0; i < pCreateInfo->enabledExtensionCount; i++)
        {
            const char *name = pCreateInfo->ppEnabledExtensionNames[i];

            if (vkl_is_own_instance_ext(name))
                continue;
            if (vkl_has_ext(props, count, name))
                names[enabled++] = name;
            else
                missing = TRUE;
        }
        vkl_free(props);

        /* with one driver its answer is the application's; with more, a
           driver without a requested extension just stays out */
        if (missing)
        {
            res = VK_ERROR_EXTENSION_NOT_PRESENT;
            continue;
        }

        info.enabledExtensionCount = enabled;
        info.ppEnabledExtensionNames = names;
        r = create(&info, pAllocator, &ii->handle);
        if (r != VK_SUCCESS)
        {
            D(bug("[Vulkan] driver %lu: vkCreateInstance failed (%ld)\n", (unsigned long)n, (long)r));
            res = r;
            continue;
        }

        ii->instance = inst;
        ii->icd = icd;
        vkl_icd_instance_fill(ii);
        inst->icdCount++;
    }

    vkl_free(names);

    if (!inst->icdCount)
    {
        vkl_free(inst);
        return res;
    }

    *pInstance = (VkInstance)inst;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator)
{
    struct vkl_instance *inst = VKL_INSTANCE(instance);
    ULONG n;

    if (!inst)
        return;

    vkl_wsi_instance_cleanup(inst);

    for (n = 0; n < inst->icdCount; n++)
    {
        struct vkl_icd_instance *ii = &inst->icd[n];
        PFN_vkDestroyInstance destroy = (PFN_vkDestroyInstance)ii->icd->gipa(ii->handle, "vkDestroyInstance");

        if (destroy)
            destroy(ii->handle, pAllocator);
    }

    inst->magic = 0;
    vkl_free(inst);
}

/* every driver's physical devices, each pointing at its driver's table */
static uint32_t vkl_collect_physdevs(struct vkl_instance *inst, VkPhysicalDevice *devs, VkResult *resp)
{
    uint32_t total = 0;
    ULONG n;

    *resp = VK_SUCCESS;
    for (n = 0; n < inst->icdCount; n++)
    {
        struct vkl_icd_instance *ii = &inst->icd[n];
        uint32_t count = 0, i;
        VkResult r;

        if (!ii->disp.EnumeratePhysicalDevices)
            continue;
        r = ii->disp.EnumeratePhysicalDevices(ii->handle, &count, NULL);
        if (r < 0)
        {
            if (inst->icdCount == 1)
                *resp = r;
            continue;
        }
        if (count > VKL_MAX_PHYSDEVS - total)
            count = VKL_MAX_PHYSDEVS - total;
        if (!count)
            continue;
        r = ii->disp.EnumeratePhysicalDevices(ii->handle, &count, devs + total);
        if (r < 0)
        {
            if (inst->icdCount == 1)
                *resp = r;
            continue;
        }
        for (i = 0; i < count; i++)
            VKL_SET_DISPATCH(devs[total + i], &ii->disp);
        total += count;
    }
    return total;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance,
    uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices)
{
    VkPhysicalDevice devs[VKL_MAX_PHYSDEVS];
    VkResult res;
    uint32_t total = vkl_collect_physdevs(VKL_INSTANCE(instance), devs, &res);

    if (res != VK_SUCCESS)
        return res;

    if (!pPhysicalDevices)
    {
        *pPhysicalDeviceCount = total;
        return VK_SUCCESS;
    }
    if (*pPhysicalDeviceCount < total)
        res = VK_INCOMPLETE;
    else
        *pPhysicalDeviceCount = total;
    memcpy(pPhysicalDevices, devs, *pPhysicalDeviceCount * sizeof(devs[0]));
    return res;
}

/* each physical device is a group of its own */
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroups(VkInstance instance,
    uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    VkPhysicalDevice devs[VKL_MAX_PHYSDEVS];
    VkResult res;
    uint32_t total = vkl_collect_physdevs(VKL_INSTANCE(instance), devs, &res), i;

    if (res != VK_SUCCESS)
        return res;

    if (!pPhysicalDeviceGroupProperties)
    {
        *pPhysicalDeviceGroupCount = total;
        return VK_SUCCESS;
    }
    if (*pPhysicalDeviceGroupCount < total)
        res = VK_INCOMPLETE;
    else
        *pPhysicalDeviceGroupCount = total;
    for (i = 0; i < *pPhysicalDeviceGroupCount; i++)
    {
        VkPhysicalDeviceGroupProperties *p = &pPhysicalDeviceGroupProperties[i];

        p->physicalDeviceCount = 1;
        memset(p->physicalDevices, 0, sizeof(p->physicalDevices));
        p->physicalDevices[0] = devs[i];
        p->subsetAllocation = VK_FALSE;
    }
    return res;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHR(VkInstance instance,
    uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties)
{
    return vkEnumeratePhysicalDeviceGroups(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

/* ---- vkGetInstanceProcAddr ---- */

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *pName)
{
    struct vkl_instance *inst = VKL_INSTANCE(instance);
    const struct vkl_command *cmd;
    PFN_vkVoidFunction proc;
    ULONG n;

    if (!pName)
        return NULL;

    /* the global commands, with or without an instance */
    if (!strcmp(pName, "vkGetInstanceProcAddr"))
        return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    if (!strcmp(pName, "vkCreateInstance"))
        return (PFN_vkVoidFunction)vkCreateInstance;
    if (!strcmp(pName, "vkEnumerateInstanceVersion"))
        return (PFN_vkVoidFunction)vkEnumerateInstanceVersion;
    if (!strcmp(pName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction)vkEnumerateInstanceExtensionProperties;
    if (!strcmp(pName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction)vkEnumerateInstanceLayerProperties;

    if (!inst || inst->magic != VKL_INSTANCE_MAGIC)
        return NULL;

    /* the library's window system entry points */
    proc = vkl_wsi_instance_proc(inst, pName);
    if (proc)
        return proc;

    cmd = vkl_find_command(pName);
    if (!cmd)
    {
        /* a command newer than the library: the first driver's own entry
           point, which works for whatever takes one of its objects */
        return inst->icd[0].icd->gipa(inst->icd[0].handle, pName);
    }

    /* the library's entry point, when a driver (or the library, which then
       put its function in the table) implements the command */
    for (n = 0; n < inst->icdCount; n++)
    {
        struct vkl_icd_instance *ii = &inst->icd[n];

        if (cmd->level == VKL_LEVEL_DEVICE)
        {
            if (ii->icd->gipa(ii->handle, pName))
                return cmd->entry;
        }
        else if (VKL_SLOT(&ii->disp, cmd))
            return cmd->entry;
    }
    return NULL;
}
