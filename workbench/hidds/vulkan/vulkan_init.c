/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Vulkan HIDD initialization — mirrors gallium_init.c.
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <hidd/vulkan_hidd.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "vulkan_intern.h"

/* Global pointer so driver HIDDs can register GPUs via VulkanHidd_RegisterGPU() */
static struct vulkanstaticdata *vulkan_global_sd = NULL;

/*
 * Public GPU registration — called by driver HIDDs at init time.
 * Exported as library function slot 5 in vulkan.hidd.
 */
AROS_LH13(int32_t, VulkanHidd_RegisterGPU,
    AROS_LHA(const char *, class_id, A0),
    AROS_LHA(OOP_Object *, pci_device, A1),
    AROS_LHA(uint32_t, vendor_id, D0),
    AROS_LHA(uint32_t, device_id, D1),
    AROS_LHA(uint32_t, flags, D2),
    AROS_LHA(uint64_t, vram_size, D3),
    AROS_LHA(const char *, name, A2),
    AROS_LHA(int32_t, priority, D4),
    AROS_LHA(const struct VulkanDriverCallbacks *, callbacks, A3),
    AROS_LHA(APTR, driver_data, A4),
    AROS_LHA(uint8_t, pci_bus, D5),
    AROS_LHA(uint8_t, pci_dev_num, D6),
    AROS_LHA(uint8_t, pci_func, D7),
    LIBBASETYPEPTR, VulkanHiddBase, 5, HiddVulkan)
{
    AROS_LIBFUNC_INIT

    if (!vulkan_global_sd)
    {
        /* Un-gated: this is the exact reason a driver's RegisterGPU silently
         * returns slot -1. It fires when a driver registers BEFORE the vulkan
         * loader has instantiated the Hidd_Vulkan object (ADD2INITLIB InitLib,
         * which publishes vulkan_global_sd, only runs then). Drivers must touch
         * the loader (e.g. vkEnumerateInstanceVersion) before registering. */
        bug("[Vulkan] RegisterGPU: REJECTED — vulkan_global_sd not published yet "
            "(loader/registry uninitialised; call a vk* entry first)\n");
        return -1;
    }

    /* Un-gated diagnostic: sd is non-NULL here, so a returned -1 means the
     * free-slot search found nothing. Dump sd, gpu_count and the first few
     * slots' in_use bits so we can see whether the registry is genuinely full,
     * uninitialised (garbage in_use), or a stale/wrong sd pointer. */
    {
        ULONG dbgi; ULONG inuse = 0;
        for (dbgi = 0; dbgi < VULKAN_MAX_GPUS; dbgi++)
            if (vulkan_global_sd->gpus[dbgi].in_use) inuse++;
        bug("[Vulkan] RegisterGPU: sd=%p gpu_count=%lu in_use=%lu/%d "
            "class='%s' pci_device=%p callbacks=%p BDF=%02x:%02x.%x\n",
            vulkan_global_sd, (unsigned long)vulkan_global_sd->gpu_count,
            (unsigned long)inuse, VULKAN_MAX_GPUS,
            class_id ? class_id : "(null)", pci_device, callbacks,
            pci_bus, pci_dev_num, pci_func);
    }

    int32_t slot = VulkanRegisterGPU(vulkan_global_sd, class_id, pci_device,
                                      vendor_id, device_id, flags,
                                      vram_size, name, priority,
                                      callbacks, driver_data,
                                      pci_bus, pci_dev_num, pci_func);

    bug("[Vulkan] RegisterGPU: '%s' (0x%04x:0x%04x) PCI %02x:%02x.%x -> slot %ld%s\n",
        name ? name : "?", vendor_id, device_id,
        pci_bus, pci_dev_num, pci_func, (long)slot,
        slot < 0 ? " [REJECTED by slot search]"
                 : (callbacks ? " [dispatch wired]" : " [no callbacks]"));

    return slot;

    AROS_LIBFUNC_EXIT
}

/*
 * Set the coarse "PSP firmware loaded" flag on a previously registered GPU,
 * keyed on (pci_bus, pci_dev, pci_func, vendor_id, device_id). Used by
 * radeonsi (and any other AMD-class driver) so independent code paths
 * for the same card — HIDD AccelInit and RADV winsys-create being the
 * two we care about today — can avoid re-running the heavy PSP firmware
 * load on a card that's already alive. Wrong-flag risk is bounded:
 * radeonsi_ensure_psp_loaded() always re-probes SOL via MP0 before
 * trusting the LOADED bit. Returns TRUE if a matching slot was updated.
 *
 * Library slot 6.
 */
AROS_LH6(BOOL, VulkanHidd_SetGPUPSPLoaded,
    AROS_LHA(uint8_t,  pci_bus,     D0),
    AROS_LHA(uint8_t,  pci_dev_num, D1),
    AROS_LHA(uint8_t,  pci_func,    D2),
    AROS_LHA(uint32_t, vendor_id,   D3),
    AROS_LHA(uint32_t, device_id,   D4),
    AROS_LHA(BOOL,     loaded,      D5),
    LIBBASETYPEPTR, VulkanHiddBase, 6, HiddVulkan)
{
    AROS_LIBFUNC_INIT

    if (!vulkan_global_sd) return FALSE;
    return VulkanSetGPUPSPLoaded(vulkan_global_sd, pci_bus, pci_dev_num,
                                 pci_func, vendor_id, device_id, loaded);

    AROS_LIBFUNC_EXIT
}

/*
 * Read the coarse "PSP firmware loaded" flag for a registered GPU.
 * Returns FALSE when no matching slot exists or when the slot has not
 * yet been marked loaded. The returned value is a snapshot — callers
 * (radeonsi_ensure_psp_loaded) must follow up with a hardware sanity
 * probe before trusting the answer for ring submission.
 *
 * Library slot 7.
 */
AROS_LH5(BOOL, VulkanHidd_IsGPUPSPLoaded,
    AROS_LHA(uint8_t,  pci_bus,     D0),
    AROS_LHA(uint8_t,  pci_dev_num, D1),
    AROS_LHA(uint8_t,  pci_func,    D2),
    AROS_LHA(uint32_t, vendor_id,   D3),
    AROS_LHA(uint32_t, device_id,   D4),
    LIBBASETYPEPTR, VulkanHiddBase, 7, HiddVulkan)
{
    AROS_LIBFUNC_INIT

    if (!vulkan_global_sd) return FALSE;
    return VulkanIsGPUPSPLoaded(vulkan_global_sd, pci_bus, pci_dev_num,
                                pci_func, vendor_id, device_id);

    AROS_LIBFUNC_EXIT
}

static int HiddVulkan_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    vulkan_global_sd = NULL;

    if (LIBBASE->sd.vulkanAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Vulkan);

    return TRUE;
}

static int HiddVulkan_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.vulkanAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Vulkan);

    if (LIBBASE->sd.vulkanAttrBase)
    {
        /* Initialize GPU registry */
        ULONG i;
        for (i = 0; i < VULKAN_MAX_GPUS; i++)
            LIBBASE->sd.gpus[i].in_use = FALSE;
        LIBBASE->sd.gpu_count = 0;
        InitSemaphore(&LIBBASE->sd.registry_lock);

        /* Publish for driver HIDDs */
        vulkan_global_sd = &LIBBASE->sd;

        D(bug("[Vulkan] InitLib: GPU registry initialized (%d slots)\n",
              VULKAN_MAX_GPUS));

        return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(HiddVulkan_InitLib, 0)
ADD2EXPUNGELIB(HiddVulkan_ExpungeLib, 0)
