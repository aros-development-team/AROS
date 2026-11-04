/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    How nvgfx.hidd's Vulkan class hands libnvk the DRM device: its DRM
    entry table (hidds/nvgfx/include/libdrm/drm_entry.h), before the
    first Vulkan entry point runs.
*/

#ifndef NVK_DRM_H
#define NVK_DRM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void nvk_drm_set_entry(const void *entry);
bool nvk_drm_available(void);

#ifdef __cplusplus
}
#endif

#endif /* NVK_DRM_H */
