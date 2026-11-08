#ifndef AMDD_GALLIUM_H
#define AMDD_GALLIUM_H
/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    radeonsi.amdd internals.
*/

#include <exec/types.h>
#include <oop/oop.h>
#include <amdd/amdd.h>

#include "util/xmlconfig.h"

struct amdgpu_drm_entry;
struct pipe_screen;
struct pipe_context;
struct pipe_resource;

/* the driver's services and DRM entry table (from struct amdd_host) */
extern struct amdd_host amdd_host;
extern const struct amdgpu_drm_entry *amdd_drm;

#define amdd_log(...) do { if (amdd_host.log) amdd_host.log(__VA_ARGS__); } while (0)

/* A radeonsi screen with a DRM client of its own */
struct amdd_screen
{
    int                 fd;
    struct pipe_screen  *screen;
    driOptionCache      option_info;
    driOptionCache      option_cache;
};

/* A scan-out buffer of the driver imported into a screen, kept while the
   bitmap keeps that buffer */
struct amdd_scanout
{
    struct pipe_resource    *res;
    ULONG                   id;
};

/* radeonsi_gallium.c */
BOOL amdd_host_set(const struct amdd_host *host);
BOOL amdd_screen_open(struct amdd_screen *s);
void amdd_screen_close(struct amdd_screen *s);
struct pipe_resource *amdd_scanout_get(struct pipe_screen *screen, OOP_Object *bm, struct amdd_scanout *so);
void amdd_screen_finish(struct pipe_screen *screen, struct pipe_context *pipe);

#endif /* AMDD_GALLIUM_H */
