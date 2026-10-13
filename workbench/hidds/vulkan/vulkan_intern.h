#ifndef _VULKAN_INTERN_H
#define _VULKAN_INTERN_H

/*
    Copyright 2025-2026, The AROS Development Team. All rights reserved.

    Internal header for the Vulkan base HIDD.

    GPU Registry:
    - Supports multiple Vulkan-capable GPUs (multi-card)
    - Each GPU is registered with its OOP class, PCI device, and priority
    - Priority ordering allows user preference for GPU selection
    - Registry entries are added by driver HIDDs at init time
*/

#include <stdint.h>
#include <stdbool.h>

#include <exec/semaphores.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/* Bumped 8 -> 32 in commit 6d-D for inference rack scale (we are
 * targeting NexOS-on-bare-metal compute servers with N>8 V620 / Mi-class
 * cards). Footprint is sizeof(VulkanGPUEntry) * MAX = ~7KB at 32 slots
 * which is trivial and avoids ever having to revisit the cap once shipped.
 * Public define lives in include/vulkan_hidd.h — keep both in sync. */
#ifndef VULKAN_MAX_GPUS
#define VULKAN_MAX_GPUS     32
#endif

/* GPU capability flags — defined in public header vulkan_hidd.h */

/* VulkanDriverCallbacks is defined in public header vulkan_hidd.h */

/* GPU registry entry — one per Vulkan-capable GPU */
struct VulkanGPUEntry
{
    BOOL            in_use;       /* TRUE if this slot is occupied */
    BOOL            locked;       /* TRUE if exclusively claimed (compute) */
    uint32_t        use_count;    /* number of active 3D device users */
    char            class_id[64]; /* OOP class ID (e.g. "hidd.vulkan.radeonsi") */
    OOP_Object     *driver_obj;   /* instantiated driver object (or NULL) */
    OOP_Object     *pci_device;   /* PCI device OOP object from pci.hidd */
    uint32_t        vendor_id;    /* PCI vendor (0x1002 = AMD) */
    uint32_t        device_id;    /* PCI device ID */
    uint32_t        flags;        /* VULKAN_GPU_* capability flags */
    int32_t         priority;     /* user preference: lower = preferred */
    uint64_t        vram_size;    /* VRAM in bytes (0 for iGPU) */
    uint8_t         pci_bus;      /* PCI bus number */
    uint8_t         pci_dev;      /* PCI device number */
    uint8_t         pci_func;     /* PCI function number */
    uint8_t         pad;
    char            name[64];     /* human-readable GPU name */
    struct VulkanDriverCallbacks callbacks; /* driver dispatch */
    APTR            driver_data;  /* opaque driver context (e.g. carddata) */
    /* Coarse cross-call substrate-load flag. TRUE iff this card has its
     * PSP firmware live (or — for non-AMD drivers — equivalent firmware
     * substrate). Set by drivers via VulkanHidd_SetGPUPSPLoaded() after
     * a successful load; consulted via VulkanHidd_IsGPUPSPLoaded() before
     * attempting redundant loads. The registry intentionally does NOT
     * carry AMD-specific PSP detail — ring/cmd/fence buffers etc. live
     * on the per-card aros_radv_winsys::psp_ctx. */
    BOOL            psp_loaded;
    BOOL            pad2[3];
};

struct HiddVulkanData
{
    APTR pad;   /* subclasses carry driver-specific data */
};

struct vulkanstaticdata
{
    OOP_Class       *vulkanclass;
    OOP_AttrBase    vulkanAttrBase;

    /* GPU registry — supports multiple Vulkan GPUs */
    struct VulkanGPUEntry  gpus[VULKAN_MAX_GPUS];
    uint32_t               gpu_count;
    struct SignalSemaphore  registry_lock;
};

LIBBASETYPE
{
    struct Library              LibNode;
    struct vulkanstaticdata     sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

/* ---- Registry API (called by driver HIDDs at init time) ---- */

/*
 * Register a Vulkan-capable GPU with the registry.
 *
 * class_id:   OOP class name (e.g. "hidd.vulkan.radeonsi")
 * pci_device: PCI device object from pci.hidd enumeration
 * vendor_id:  PCI vendor ID
 * device_id:  PCI device ID
 * flags:      VULKAN_GPU_* flags
 * vram_size:  VRAM in bytes (0 if unknown or iGPU)
 * name:       human-readable GPU name
 * priority:   initial priority (0 = highest, lower = preferred)
 *
 * Returns slot index (0..VULKAN_MAX_GPUS-1) or -1 on failure.
 */
static inline int32_t VulkanRegisterGPU(struct vulkanstaticdata *sd,
                                         const char *class_id,
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
                                         uint8_t pci_func)
{
    uint32_t i;
    int32_t slot = -1;

    ObtainSemaphore(&sd->registry_lock);

    /* Idempotency: if a card with the same (BDF + vendor + device) is
     * already registered, refresh its mutable fields and return that
     * slot. Drivers re-running their PCI scan (e.g. AccelInit followed
     * later by RADV winsys-create on the same card) MUST NOT consume
     * a second slot — at N-card scale that silently exhausts the table
     * and fragments the lookup-by-BDF answer. */
    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        struct VulkanGPUEntry *e = &sd->gpus[i];
        if (e->in_use &&
            e->pci_bus == pci_bus && e->pci_dev == pci_dev_num &&
            e->pci_func == pci_func &&
            e->vendor_id == vendor_id && e->device_id == device_id) {
            slot = (int32_t)i;
            break;
        }
    }

    if (slot < 0) {
        for (i = 0; i < VULKAN_MAX_GPUS; i++) {
            if (!sd->gpus[i].in_use) {
                slot = (int32_t)i;
                break;
            }
        }
    }

    if (slot >= 0) {
        struct VulkanGPUEntry *e = &sd->gpus[slot];
        BOOL was_in_use = e->in_use;
        e->in_use      = TRUE;
        if (!was_in_use) {
            e->locked      = FALSE;
            e->use_count   = 0;
            e->driver_obj  = NULL;
            e->psp_loaded  = FALSE;  /* fresh slot starts with no substrate */
        }
        e->pci_device  = pci_device;
        e->pci_bus     = pci_bus;
        e->pci_dev     = pci_dev_num;
        e->pci_func    = pci_func;
        e->vendor_id   = vendor_id;
        e->device_id   = device_id;
        e->flags       = flags;
        e->priority    = priority;
        e->vram_size   = vram_size;
        e->driver_data = driver_data;

        /* Store driver callbacks */
        if (callbacks) {
            e->callbacks = *callbacks;
        } else {
            uint32_t k;
            char *p = (char *)&e->callbacks;
            for (k = 0; k < sizeof(e->callbacks); k++) p[k] = 0;
        }

        /* Copy strings safely */
        {
            const char *s;
            uint32_t j;

            s = class_id ? class_id : "";
            for (j = 0; j < sizeof(e->class_id) - 1 && s[j]; j++)
                e->class_id[j] = s[j];
            e->class_id[j] = '\0';

            s = name ? name : "Unknown GPU";
            for (j = 0; j < sizeof(e->name) - 1 && s[j]; j++)
                e->name[j] = s[j];
            e->name[j] = '\0';
        }

        if (!was_in_use)
            sd->gpu_count++;
    }

    ReleaseSemaphore(&sd->registry_lock);

    return slot;
}

/*
 * Look up a registered GPU by PCI BDF + vendor + device. Returns a
 * pointer to the entry under shared lock (the caller must finish using
 * it before any other writer mutates the registry — currently only
 * VulkanRegisterGPU and VulkanSetGPUPSPLoaded write fields and only
 * the mutable substrate fields move, so reads of (vendor/device/BDF/
 * psp_loaded) are safe by-value snapshots after this returns).
 *
 * Used by drivers' ensure-substrate-loaded path to consult the coarse
 * cross-call psp_loaded flag without re-running the heavy load.
 *
 * Returns NULL if no match.
 */
static inline struct VulkanGPUEntry *
VulkanFindGPUByBDF(struct vulkanstaticdata *sd,
                   uint8_t pci_bus, uint8_t pci_dev_num, uint8_t pci_func,
                   uint32_t vendor_id, uint32_t device_id)
{
    uint32_t i;
    struct VulkanGPUEntry *found = NULL;

    if (!sd) return NULL;

    ObtainSemaphoreShared(&sd->registry_lock);
    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        struct VulkanGPUEntry *e = &sd->gpus[i];
        if (e->in_use &&
            e->pci_bus == pci_bus && e->pci_dev == pci_dev_num &&
            e->pci_func == pci_func &&
            e->vendor_id == vendor_id && e->device_id == device_id) {
            found = e;
            break;
        }
    }
    ReleaseSemaphore(&sd->registry_lock);

    return found;
}

/*
 * Set/clear the coarse psp_loaded flag on a previously registered GPU.
 * Returns TRUE if a matching slot was found and updated, FALSE otherwise.
 * Cheap; safe to call from any context that already holds no registry lock.
 */
static inline BOOL
VulkanSetGPUPSPLoaded(struct vulkanstaticdata *sd,
                      uint8_t pci_bus, uint8_t pci_dev_num, uint8_t pci_func,
                      uint32_t vendor_id, uint32_t device_id,
                      BOOL loaded)
{
    uint32_t i;
    BOOL updated = FALSE;

    if (!sd) return FALSE;

    ObtainSemaphore(&sd->registry_lock);
    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        struct VulkanGPUEntry *e = &sd->gpus[i];
        if (e->in_use &&
            e->pci_bus == pci_bus && e->pci_dev == pci_dev_num &&
            e->pci_func == pci_func &&
            e->vendor_id == vendor_id && e->device_id == device_id) {
            e->psp_loaded = loaded ? TRUE : FALSE;
            updated = TRUE;
            break;
        }
    }
    ReleaseSemaphore(&sd->registry_lock);
    return updated;
}

/*
 * Read the coarse psp_loaded flag. Returns FALSE if no matching slot.
 */
static inline BOOL
VulkanIsGPUPSPLoaded(struct vulkanstaticdata *sd,
                     uint8_t pci_bus, uint8_t pci_dev_num, uint8_t pci_func,
                     uint32_t vendor_id, uint32_t device_id)
{
    BOOL result = FALSE;
    uint32_t i;

    if (!sd) return FALSE;

    ObtainSemaphoreShared(&sd->registry_lock);
    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        struct VulkanGPUEntry *e = &sd->gpus[i];
        if (e->in_use &&
            e->pci_bus == pci_bus && e->pci_dev == pci_dev_num &&
            e->pci_func == pci_func &&
            e->vendor_id == vendor_id && e->device_id == device_id) {
            result = e->psp_loaded;
            break;
        }
    }
    ReleaseSemaphore(&sd->registry_lock);
    return result;
}

/*
 * Find the preferred (lowest priority) GPU that is not exclusively
 * locked for compute.  Among equal priority, prefer lower use_count.
 * Returns pointer to entry or NULL if registry is empty.
 */
static inline struct VulkanGPUEntry *
VulkanGetPreferredGPU(struct vulkanstaticdata *sd)
{
    struct VulkanGPUEntry *best = NULL;
    uint32_t i;

    ObtainSemaphoreShared(&sd->registry_lock);

    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        if (sd->gpus[i].in_use && !sd->gpus[i].locked) {
            if (!best ||
                sd->gpus[i].priority < best->priority ||
                (sd->gpus[i].priority == best->priority &&
                 sd->gpus[i].use_count < best->use_count))
                best = &sd->gpus[i];
        }
    }

    ReleaseSemaphore(&sd->registry_lock);
    return best;
}

/*
 * Find the preferred display-capable GPU.
 *
 * Presentation paths should not select headless/compute/vGPU entries
 * while a display-capable GPU is available.
 */
static inline struct VulkanGPUEntry *
VulkanGetPreferredDisplayGPU(struct vulkanstaticdata *sd)
{
    struct VulkanGPUEntry *best = NULL;
    uint32_t i;

    ObtainSemaphoreShared(&sd->registry_lock);

    for (i = 0; i < VULKAN_MAX_GPUS; i++) {
        if (sd->gpus[i].in_use &&
            !sd->gpus[i].locked &&
            !(sd->gpus[i].flags & (VULKAN_GPU_HEADLESS |
                                   VULKAN_GPU_VGPU |
                                   VULKAN_GPU_COMPUTE))) {
            if (!best ||
                sd->gpus[i].priority < best->priority ||
                (sd->gpus[i].priority == best->priority &&
                 sd->gpus[i].use_count < best->use_count))
                best = &sd->gpus[i];
        }
    }

    ReleaseSemaphore(&sd->registry_lock);

    if (best)
        return best;

    return VulkanGetPreferredGPU(sd);
}

#endif /* _VULKAN_INTERN_H */
