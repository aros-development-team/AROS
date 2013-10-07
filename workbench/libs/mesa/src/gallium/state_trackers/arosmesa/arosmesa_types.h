/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_TYPES_H
#define AROSMESA_TYPES_H

#include <GL/arosmesa.h>
#include "main/mtypes.h"
#include "state_tracker/st_api.h"

struct arosmesa_framebuffer
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

/* AROSMesa context */
struct arosmesa_context
{
    struct st_context_iface     *st;
    struct st_visual            stvis;
    struct st_manager           *stmanager;

    struct arosmesa_framebuffer *framebuffer;

    struct Window               *window;                /* Intuition window */
    struct Screen               *Screen;
    ULONG                       BitsPerPixel;
    
    /* Rastport 'visible' to user (window rasport, screen rastport)*/
    struct RastPort             *visible_rp;
    /* Rastport dimentions */
    ULONG                       visible_rp_width;       /* the rastport drawing area full size*/
    ULONG                       visible_rp_height;      /* the rastport drawing area full size*/

    /* Buffer information */
    ULONG                      top, bottom;            /* offsets due to window border */
    ULONG                      left, right;            /* offsets due to window border */    
};

/* GL API for state tracker */
extern struct st_api * glstapi;

#endif /* AROSMESA_TYPES_H */
