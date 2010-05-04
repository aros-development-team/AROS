/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(I915_TRACE_H)
#define I915_TRACE_H

#include "drmP.h"

static inline void trace_i915_gem_object_create(struct drm_gem_object *obj)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_complete(struct drm_device *dev, u32 seqno)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_destroy(struct drm_gem_object *obj)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_unbind(struct drm_gem_object *obj)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_change_domain(struct drm_gem_object *obj, 
                                    uint32_t old_read_domains, uint32_t old_write_domain)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_clflush(struct drm_gem_object *obj)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_flush(struct drm_device *dev, u32 seqno,
                                    u32 flush_domains, u32 invalidate_domains)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_wait_begin(struct drm_device *dev, u32 seqno)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_wait_end(struct drm_device *dev, u32 seqno)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_retire(struct drm_device *dev, u32 seqno)
{
    TRACE("\n");
}

static inline void trace_i915_ring_wait_begin(struct drm_device *dev)
{
    TRACE("\n");
}

static inline void trace_i915_ring_wait_end(struct drm_device *dev)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_bind(struct drm_gem_object *obj, u32 gtt_offset)
{
    TRACE("\n");
}

static inline void trace_i915_gem_request_submit(struct drm_device *dev, u32 seqno)
{
    TRACE("\n");
}

static inline void trace_i915_gem_object_get_fence(struct drm_gem_object *obj,
                                    int fance, int tiling_mode)
{
    TRACE("\n");
}
#endif /* I915_TRACE_H */

