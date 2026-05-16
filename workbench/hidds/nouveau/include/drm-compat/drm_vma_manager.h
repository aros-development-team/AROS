/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DRM_COMPAT_DRM_VMA_MANAGER_H
#define DRM_COMPAT_DRM_VMA_MANAGER_H

/* AROS has single addres space. No need for mapping between kernel addresses and user process virtual addresses */

struct drm_vma_offset_manager
{
    ULONG dummy;
};

#define drm_vma_node_reset(x)
#define drm_vma_offset_manager_init(x, y, z)

#endif
