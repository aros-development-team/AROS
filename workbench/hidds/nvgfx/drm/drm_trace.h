/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
/* The DRM core's tracepoints are not built; the vblank ones compile away. */
#ifndef _AROS_DRM_TRACE_H_
#define _AROS_DRM_TRACE_H_

#define trace_drm_vblank_event(pipe, seq, time, high_prec)	do { } while (0)
#define trace_drm_vblank_event_queued(file, pipe, seq)		do { } while (0)
#define trace_drm_vblank_event_delivered(file, pipe, seq)	do { } while (0)

#endif
