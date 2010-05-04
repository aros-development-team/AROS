/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id: arosdrm_intel.c 32778 2010-03-12 20:19:32Z deadwood $
*/

#include "arosdrm.h"
#include "drmP.h"

extern struct drm_driver * current_drm_driver;

/* FIXME: Array for now, list maybe in future */
extern struct drm_file * drm_files[128];

#include "i915_drm.h"
#include "i915_drv.h"

int i915_getparam(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);

int drmIntelIoctlEmul(int fildes, int request, void * arg)
{
    int ret = -EINVAL;
    
    if (!drm_files[fildes])
        return ret;

    switch(request)
    {
    case(DRM_IOCTL_I915_GEM_GET_APERTURE):
        return i915_gem_get_aperture_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GETPARAM):
        return i915_getparam(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_CREATE):
        return i915_gem_create_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_MMAP):
        return i915_gem_mmap_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_SET_DOMAIN):
        return i915_gem_set_domain_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_SW_FINISH):
        return i915_gem_sw_finish_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_SET_TILING):
        return i915_gem_set_tiling(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_PWRITE):
        return i915_gem_pwrite_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_EXECBUFFER):
        return i915_gem_execbuffer(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_MADVISE):
        return i915_gem_madvise_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_I915_GEM_BUSY):
        return i915_gem_busy_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    case(DRM_IOCTL_GEM_CLOSE):
        return drm_gem_close_ioctl(current_drm_driver->dev, arg, drm_files[fildes]);
    default:
        asm("int3");
        DRM_IMPL("IOCTL: %d -> %d\n", fildes, request);
        break;
    }
    
    return ret;
}

