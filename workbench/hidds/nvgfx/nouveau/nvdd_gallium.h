#ifndef NVDD_GALLIUM_H
#define NVDD_GALLIUM_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    nouveau.nvdd internals.
*/

struct nouveau_drm_entry;

/* the driver's DRM entry table and log (from struct nvdd_host) */
extern const struct nouveau_drm_entry *nvdd_drm;
extern void (*nvdd_log)(const char *fmt, ...);

#endif
