/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <drm-compat/drm_compat_types.h>

#include "nouveau_drv.h"
#include "nouveau_bo.h"
#include "nouveau_gem.h"

void *drm_gem_nouveau_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
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
        if (!nvbo->kmap.virtual)
            nouveau_bo_map(nvbo);

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
            nouveau_bo_unmap(nvbo);
    }

    /* Release the acquired reference */
    mutex_lock(&dev->struct_mutex);
    drm_gem_object_put(gem_object);
    mutex_unlock(&dev->struct_mutex);
}
