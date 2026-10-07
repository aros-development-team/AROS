/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_OF_H_
#define _DRM_OF_H_

/* no device tree graph here */
struct drm_device;
struct device_node;
struct drm_encoder;
struct drm_bridge;
struct drm_panel;
static inline uint32_t drm_of_find_possible_crtcs(struct drm_device *dev, struct device_node *port) { return 0; }
static inline int drm_of_find_panel_or_bridge(const struct device_node *np, int port, int endpoint, struct drm_panel **panel, struct drm_bridge **bridge) { return -ENODEV; }
static inline uint32_t drm_of_crtc_port_mask(struct drm_device *dev, struct device_node *port) { return 0; }

#endif
