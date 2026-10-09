/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_GEM_ATOMIC_HELPER_H_
#define _DRM_GEM_ATOMIC_HELPER_H_

#include <linux/iosys-map.h>
#include <drm/drm_plane.h>

/* the plane-fb fence wait: buffers here are always idle when they reach
   the display path, so there is nothing to wait for */
struct drm_simple_display_pipe;

static inline int drm_gem_plane_helper_prepare_fb(struct drm_plane *plane, struct drm_plane_state *state) { return 0; }

#endif
