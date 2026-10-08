/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <exec/memory.h>

#include <linux/kernel.h>
#include <linux/slab.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_utils.h>
#include <drm/display/drm_hdcp_helper.h>
#include <drm/ttm/ttm_bo.h>

#include "amdgpu.h"
#include "amdgpu_object.h"
#include "amdgpu_xcp_drv.h"
#include "amdgpu_mode.h"
#include <drm/drm_crtc.h>
#include <drm/drm_vblank.h>

/* --- allocator behind kmalloc ------------------------------------------- */

#define AMDGPU_ALLOC_HEADER    16

APTR HIDDAmdgpuAlloc(ULONG size)
{
    IPTR total = (IPTR)size + AMDGPU_ALLOC_HEADER;
    IPTR *memory = AllocMem(total, MEMF_PUBLIC | MEMF_CLEAR);

    if (memory == NULL)
        return NULL;

    memory[0] = total;
    return (APTR)((UBYTE *)memory + AMDGPU_ALLOC_HEADER);
}

VOID HIDDAmdgpuFree(APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - AMDGPU_ALLOC_HEADER);
        FreeMem(real, real[0]);
    }
}

IPTR HIDDAmdgpuAllocSize(CONST_APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *)((UBYTE *)memory - AMDGPU_ALLOC_HEADER);
        return real[0] - AMDGPU_ALLOC_HEADER;
    }
    return 0;
}

/* --- buffer object mappings for the hidd and the winsys ------------------ */

/*
 * The CPU view of a buffer is one linear kernel mapping, so the buffer is
 * pinned contiguous in its domain before it is mapped: VRAM is otherwise
 * handed out in blocks, and display would move it on first scanout. The
 * mapping and the pin last until the buffer is unmapped.
 */
void *drm_gem_amdgpu_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    struct drm_gem_object *gobj;
    struct amdgpu_bo *bo;
    void *addr = NULL;

    if (!f)
        return NULL;

    gobj = drm_gem_object_lookup(f, handle);
    if (!gobj)
        return NULL;

    bo = gem_to_amdgpu_bo(gobj);
    if (amdgpu_bo_reserve(bo, false) == 0)
    {
        addr = amdgpu_bo_kptr(bo);
        if (!addr)
        {
            u32 domain = bo->preferred_domains & AMDGPU_GEM_DOMAIN_VRAM ?
                         AMDGPU_GEM_DOMAIN_VRAM : AMDGPU_GEM_DOMAIN_GTT;

            bo->flags |= AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;
            if (amdgpu_bo_pin(bo, domain) == 0)
            {
                if (amdgpu_bo_kmap(bo, &addr))
                {
                    addr = NULL;
                    amdgpu_bo_unpin(bo);
                }
            }
        }
        amdgpu_bo_unreserve(bo);
    }

    drm_gem_object_put(gobj);
    return addr;
}

void drm_gem_amdgpu_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    struct drm_gem_object *gobj;
    struct amdgpu_bo *bo;

    if (!f)
        return;

    gobj = drm_gem_object_lookup(f, handle);
    if (!gobj)
        return;

    bo = gem_to_amdgpu_bo(gobj);
    if (amdgpu_bo_reserve(bo, false) == 0)
    {
        if (amdgpu_bo_kptr(bo))
        {
            amdgpu_bo_kunmap(bo);
            amdgpu_bo_unpin(bo);
        }
        amdgpu_bo_unreserve(bo);
    }

    drm_gem_object_put(gobj);
}

/* --- pieces of the DRM core not carried here ----------------------------- */

vm_fault_t ttm_bo_vm_dummy_page(struct vm_fault *vmf, pgprot_t prot)
{
    return VM_FAULT_SIGBUS;
}

struct drm_gem_object *drm_gem_fb_get_obj(struct drm_framebuffer *fb, unsigned int plane)
{
    if (plane >= ARRAY_SIZE(fb->obj))
        return NULL;
    return fb->obj[plane];
}

const struct drm_panel_backlight_quirk *drm_get_panel_backlight_quirk(const struct drm_edid *edid)
{
    return ERR_PTR(-ENOENT);
}

int drm_connector_attach_content_protection_property(struct drm_connector *connector, bool hdcp_content_type)
{
    return 0;
}

void drm_hdcp_update_content_protection(struct drm_connector *connector, u64 val)
{
}

int drm_dev_wedged_event(struct drm_device *dev, unsigned long method, struct drm_wedge_task_info *info)
{
    drm_err(dev, "device wedged, recovery method 0x%lx\n", method);
    return 0;
}

struct dmem_cgroup_region *drmm_cgroup_register_region(struct drm_device *dev, const char *region_name, u64 size)
{
    return NULL;
}

int drm_memory_stats_is_zero(const struct drm_memory_stats *stats)
{
    return stats->shared == 0 && stats->private == 0 && stats->resident == 0 &&
           stats->purgeable == 0 && stats->active == 0;
}

void drm_file_err(struct drm_file *file_priv, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printk(KERN_ERR "[drm] %s", buf);
}

/* --- compute partitions are not exposed as separate devices -------------- */

int amdgpu_xcp_drm_dev_alloc(struct drm_device **ddev)
{
    return -ENODEV;
}

void amdgpu_xcp_drm_dev_free(struct drm_device *ddev)
{
}

void amdgpu_xcp_drv_release(void)
{
}

/* --- what the hidd asks about the device --------------------------------- */

extern struct drm_device *current_drm_device;

BOOL amdgpu_aros_has_3d(void)
{
    return current_drm_device != NULL;
}

BOOL amdgpu_aros_has_mob(void)
{
    return current_drm_device != NULL;
}

void amdgpu_aros_reset(void)
{
}

unsigned long compat_sleep_usecs(unsigned long usecs);

/* Wait for the start of the next vertical blank of a CRTC. */
int amdgpu_aros_wait_vblank(unsigned int crtc_id)
{
    struct drm_device *dev = current_drm_device;
    const struct drm_display_mode *mode;
    struct drm_crtc *crtc;
    unsigned long long line_ns;
    int vpos, hpos, flags, tries;

    if (!dev)
        return -1;
    crtc = drm_crtc_find(dev, NULL, crtc_id);
    if (!crtc || !crtc->state || !crtc->state->active)
        return -1;
    mode = &crtc->state->adjusted_mode;
    if (!mode->crtc_clock || !mode->crtc_htotal)
        return -1;
    line_ns = (unsigned long long)mode->crtc_htotal * 1000000ULL / mode->crtc_clock;

    for (tries = 0; tries < 1000; tries++)
    {
        flags = amdgpu_display_get_crtc_scanoutpos(dev, drm_crtc_index(crtc), GET_DISTANCE_TO_VBLANKSTART,
                                                   &vpos, &hpos, NULL, NULL, mode);
        if (!(flags & DRM_SCANOUTPOS_VALID))
            return -1;
        if (!(flags & DRM_SCANOUTPOS_IN_VBLANK))
            break;
        udelay(20);
    }

    for (tries = 0; tries < 10000; tries++)
    {
        unsigned long usecs;

        flags = amdgpu_display_get_crtc_scanoutpos(dev, drm_crtc_index(crtc), GET_DISTANCE_TO_VBLANKSTART,
                                                   &vpos, &hpos, NULL, NULL, mode);
        if (!(flags & DRM_SCANOUTPOS_VALID))
            return -1;
        if (flags & DRM_SCANOUTPOS_IN_VBLANK)
            return 0;
        usecs = (unsigned long)(-vpos * line_ns / 1000);
        if (usecs > 300)
            compat_sleep_usecs(usecs - 200);
        else
            udelay(20);
    }
    return -1;
}

