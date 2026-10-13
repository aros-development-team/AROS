/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef MESA3DGL_TYPES_H
#define MESA3DGL_TYPES_H

#include <GL/gla.h>
#include "main/mtypes.h"
/* Mesa 21.0+ renamed st_api.h -> frontend/api.h. __has_include is
 * reliable here; a #define probe may not be pulled in yet. */
/* Mesa >= 22: the state tracker is driven directly through st_api_*() and the
 * frontend screen/drawable structures; there is no st_api object any more. */
#include "frontend/api.h"
#include "state_tracker/st_context.h"

struct mesa3dgl_framebuffer
{
    struct pipe_frontend_drawable base;
    struct st_visual            stvis;

    struct pipe_screen          *screen;
    struct pipe_resource        *textures[ST_ATTACHMENT_COUNT];
    struct pipe_resource        *render_resource; /* Resource with results of rendering (back buffer) */

    ULONG                       width;
    ULONG                       height;
    BOOL                        resized;
};

/* mesa/gallium GL context */
struct mesa3dgl_context
{
    APTR                                driver;
    struct st_context           *st;
    struct st_visual            stvis;
    struct pipe_frontend_screen *stmanager;

    struct mesa3dgl_framebuffer *framebuffer;
    struct Window               *window;
    struct Screen               *Screen;
    ULONG                       BitsPerPixel;
    struct RastPort             *visible_rp;
    ULONG                       visible_rp_width;
    ULONG                       visible_rp_height;
    ULONG                      top, bottom;
    ULONG                      left, right;

};

/*  state trackers GL API */

#endif /* MESA3DGL_TYPES_H */
