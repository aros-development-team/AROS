/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_types.h>

#include "nouveau_drv.h"
#include "nouveau_bo.h"
#include "nouveau_gem.h"

void *drm_gem_nouveau_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle, VOID (*unmapped)(APTR), APTR data)
{
    struct drm_gem_object * gem_object = NULL;
    struct nouveau_bo * nvbo = NULL;
    void * addr = NULL;

    if (!f)
        return NULL;

    /* Get GEM objects from handle */
    gem_object = drm_gem_object_lookup(f, handle);
    if (!gem_object)
        return NULL;

    /* Translate to nouveau_bo */
    nvbo = nouveau_gem_object(gem_object);

    if (nvbo)
    {
        /* Perform mapping if not already done */
        if (!nvbo->kmap.virtual || nvbo->was_gpu_unmapped)
        {
            nouveau_bo_map(nvbo);
            nvbo->was_gpu_unmapped = FALSE;

            if (nvbo->gpu_unmapped_data != NULL && nvbo->gpu_unmapped_data != data)
                DRM_ERROR("gpu_unmapped_data already set for nvbo %p\n", nvbo);

            nvbo->gpu_unmapped      = unmapped;
            nvbo->gpu_unmapped_data = data;
        }

        addr = nvbo->kmap.virtual;
    }

    /* Release the acquired reference */
    mutex_lock(&dev->struct_mutex);
    drm_gem_object_put(gem_object);
    mutex_unlock(&dev->struct_mutex);

    /* Return virtual address */
    return addr;
}

void drm_gem_nouveau_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    struct drm_gem_object * gem_object = NULL;
    struct nouveau_bo * nvbo = NULL;

    if (!f) return;

    /* Get GEM objects from handle */
    gem_object = drm_gem_object_lookup(f, handle);
    if (!gem_object) return;

    /* Translate to nouveau_bo */
    nvbo = nouveau_gem_object(gem_object);
    if (nvbo)
    {
        /* Perform unmapping if not already done */
        if (nvbo->kmap.virtual)
        {
            nouveau_bo_unmap(nvbo);
            nvbo->was_gpu_unmapped = FALSE;
            nvbo->gpu_unmapped = NULL;
            nvbo->gpu_unmapped_data = NULL;
        }
    }

    /* Release the acquired reference */
    mutex_lock(&dev->struct_mutex);
    drm_gem_object_put(gem_object);
    mutex_unlock(&dev->struct_mutex);
}

/*
    On NV50+ cards there are two mappings needed for CPU to access GPU buffer object. First is mapping of buffer object
    into CPU's address space, second is a mapping of GPU VRAM page into that CPU address space.

    When userspace maps the object both of those mappings are executed. However if later and object is moved in VRAM,
    the GPU mapping is removed. At the same time the CPU address space pointer held in userspace remains unchanged. When
    userspace tries to access buffer object through that pointer, a page fault is raised and a special handler in TTM
    causes the GPU page to be mapped back into CPU address space.

    On AROS we don't handle page faults like that so we need a workaround. When GPU pages are unmapped but there
    was a userspace space mapping, immediatelly invalidate that mapping in userspace. This way, the next time
    userspace will try to access mapping it will find NULL and will request regular buffer object mapping which will
    establish new CPU address and map GPU pages to it.

*/
void drm_nouveau_check_userspace_mapped(struct ttm_buffer_object *bo)
{
    struct nouveau_bo *nvbo = nouveau_bo(bo);
    if (nvbo->kmap.virtual && !nvbo->was_gpu_unmapped)
    {
        nvbo->was_gpu_unmapped = TRUE;
        if (nvbo->gpu_unmapped) nvbo->gpu_unmapped(nvbo->gpu_unmapped_data);
    }
}
