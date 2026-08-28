/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_LEASE_H_
#define _DRM_LEASE_H_

/* no leasing: every object is visible to every client */
struct drm_file;
struct drm_device;
struct drm_master;

static inline bool drm_lease_held(struct drm_file *file_priv, int id) { return true; }
static inline bool _drm_lease_held(struct drm_file *file_priv, int id) { return true; }
static inline void drm_lease_revoke(struct drm_master *master) { }
static inline void drm_lease_destroy(struct drm_master *lessee) { }
static inline uint32_t drm_lease_filter_crtcs(struct drm_file *file_priv, uint32_t crtcs) { return crtcs; }
static inline uint32_t drm_lease_filter_encoders(struct drm_file *file_priv, uint32_t encoders) { return encoders; }

#endif /* _DRM_LEASE_H_ */
