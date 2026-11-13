/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_TRACE_H_
#define _DRM_TRACE_H_

/* The DRM core tracepoints; nothing listens on AROS. */

#include <linux/types.h>
#include <linux/ktime.h>

struct drm_file;

static inline void trace_drm_vblank_event(int pipe, unsigned int seq, ktime_t time, bool high_prec) { }
static inline void trace_drm_vblank_event_queued(struct drm_file *file, int pipe, unsigned int seq) { }
static inline void trace_drm_vblank_event_delivered(struct drm_file *file, int pipe, unsigned int seq) { }

#endif /* _DRM_TRACE_H_ */
