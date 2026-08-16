/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/kernel.h>

#include "nouveau_drv.h"
#include "nouveau_bo.h"
#include "nouveau_gem.h"
#include "nouveau_connector.h"

#include <drm/drm_edid.h>

/*
 * The hidd holds the CPU mapping of a buffer object directly: there is no
 * page fault to re-establish it after the object moves. So a move marks
 * the mapping stale and tells the holder, and the next map request makes
 * a fresh one.
 */

void *drm_gem_nouveau_mmap(struct drm_device *dev, struct drm_file *f, uint32_t handle, VOID (*unmapped)(APTR), APTR data)
{
    struct drm_gem_object *gem_object;
    struct nouveau_bo *nvbo;
    void *addr = NULL;

    if (!f)
        return NULL;

    gem_object = drm_gem_object_lookup(f, handle);
    if (!gem_object)
        return NULL;

    nvbo = nouveau_gem_object(gem_object);
    if (nvbo)
    {
        if (!nvbo->kmap.virtual || nvbo->was_gpu_unmapped)
        {
            if (nvbo->kmap.virtual)
                ttm_bo_kunmap(&nvbo->kmap);
            nouveau_bo_map(nvbo);
            nvbo->was_gpu_unmapped = false;

            if (nvbo->gpu_unmapped_data != NULL && nvbo->gpu_unmapped_data != data)
                DRM_ERROR("gpu_unmapped_data already set for nvbo %p\n", nvbo);

            nvbo->gpu_unmapped      = unmapped;
            nvbo->gpu_unmapped_data = data;
        }

        addr = nvbo->kmap.virtual;
    }

    drm_gem_object_put(gem_object);
    return addr;
}

void drm_gem_nouveau_munmap(struct drm_device *dev, struct drm_file *f, uint32_t handle)
{
    struct drm_gem_object *gem_object;
    struct nouveau_bo *nvbo;

    if (!f)
        return;

    gem_object = drm_gem_object_lookup(f, handle);
    if (!gem_object)
        return;

    nvbo = nouveau_gem_object(gem_object);
    if (nvbo && nvbo->kmap.virtual)
    {
        nouveau_bo_unmap(nvbo);
        nvbo->was_gpu_unmapped = false;
        nvbo->gpu_unmapped = NULL;
        nvbo->gpu_unmapped_data = NULL;
    }

    drm_gem_object_put(gem_object);
}

void drm_nouveau_check_userspace_mapped(struct ttm_buffer_object *bo)
{
    struct nouveau_bo *nvbo = nouveau_bo(bo);

    if (nvbo->kmap.virtual && !nvbo->was_gpu_unmapped)
    {
        nvbo->was_gpu_unmapped = true;
        if (nvbo->gpu_unmapped)
            nvbo->gpu_unmapped(nvbo->gpu_unmapped_data);
    }
}

/*
    The name the monitor gives for itself, from the descriptor block in its
    EDID. Not every display fills that block in, so this can succeed in
    reaching the connector and still have no name to report.
*/
BOOL drm_nouveau_get_monitor_name(struct drm_device *dev, uint32_t connector_id, char *name, int namelen)
{
    struct drm_connector *connector;
    struct drm_connector_list_iter conn_iter;
    BOOL found = FALSE;

    if (!dev || !name || (namelen < 2))
        return FALSE;

    name[0] = '\0';

    drm_connector_list_iter_begin(dev, &conn_iter);
    drm_for_each_connector_iter(connector, &conn_iter)
    {
        struct nouveau_connector *nv_connector;

        if (connector->base.id != connector_id)
            continue;

        nv_connector = nouveau_connector(connector);
        if (nv_connector->edid)
            drm_edid_get_monitor_name(nv_connector->edid, name, namelen);

        found = (name[0] != '\0');
        break;
    }
    drm_connector_list_iter_end(&conn_iter);

    return found;
}
