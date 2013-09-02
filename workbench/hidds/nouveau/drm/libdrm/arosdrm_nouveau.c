/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosdrm.h"
#include "drmP.h"

extern struct drm_driver * current_drm_driver;

/* FIXME: Array for now, list maybe in future */
extern struct drm_file * drm_files[128];

/* FIXME: this should be generic, not nouveau specific */
#include "nouveau_drv.h"
void * drmMMap(int fd, uint32_t handle)
{
    struct drm_file * f = drm_files[fd];
    struct drm_gem_object * gem_object = NULL;
    struct nouveau_bo * nvbo = NULL;
    void * addr = NULL;
    
    if (!f)
        return NULL;
    
    /* Get GEM objects from handle */
    gem_object = drm_gem_object_lookup(current_drm_driver->dev, f, handle);
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
    mutex_lock(&current_drm_driver->dev->struct_mutex);
    drm_gem_object_unreference(gem_object);
    mutex_unlock(&current_drm_driver->dev->struct_mutex);    
    
    /* Return virtual address */
    return addr;
}

void drmMUnmap(int fd, uint32_t handle)
{
    struct drm_file * f = drm_files[fd];
    struct drm_gem_object * gem_object = NULL;
    struct nouveau_bo * nvbo = NULL;
   
    if (!f) return ;
    
    /* Get GEM objects from handle */
    gem_object = drm_gem_object_lookup(current_drm_driver->dev, f, handle);
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
    mutex_lock(&current_drm_driver->dev->struct_mutex);
    drm_gem_object_unreference(gem_object);
    mutex_unlock(&current_drm_driver->dev->struct_mutex);
}

