/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef MESA3DGL_TYPES_H
#define MESA3DGL_TYPES_H

#include <GL/gla.h>
#include "main/mtypes.h"
#include "state_tracker/st_api.h"

struct mesa3dgl_framebuffer
{
    struct st_framebuffer_iface base;
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
    struct st_context_iface     *st;
    struct st_visual            stvis;
    struct st_manager           *stmanager;

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
extern struct st_api * glstapi;

#endif /* MESA3DGL_TYPES_H */
