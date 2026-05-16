/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DRM_COMPAT_DRM_VMA_MANAGER_H
#define DRM_COMPAT_DRM_VMA_MANAGER_H

#include <exec/types.h>

/* AROS has single addres space. No need for mapping between kernel addresses and user process virtual addresses */

struct drm_vma_offset_manager
{
    ULONG dummy;
};

#define drm_vma_node_reset(x)
#define drm_vma_offset_manager_init(x, y, z)
#define drm_vma_node_allow(x, y) (0) /* Always allow */
/* mmap() implementations will not use the offset_addr, they will directly return "kernel" mapping */
#define drm_vma_node_offset_addr(x) (-1L)

#endif
