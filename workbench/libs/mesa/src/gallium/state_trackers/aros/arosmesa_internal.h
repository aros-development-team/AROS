/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_INTERNAL_H
#define AROSMESA_INTERNAL_H

#include "main/mtypes.h"
#include "state_tracker/st_context.h"

#include <GL/arosmesa.h>

extern struct Library * AROSMesaCyberGfxBase;
#define CyberGfxBase AROSMesaCyberGfxBase

/* AROS frame buffer */
struct arosmesa_framebuffer
{
    struct st_framebuffer *stfb;                    /* Base class - must be first */
};

typedef struct arosmesa_framebuffer * AROSMesaFrameBuffer;

#define GET_GL_FB_PTR(arosmesa_fb) (&arosmesa_fb->stfb->Base)
#define GET_AROS_FB_PTR(gl_fb) ((AROSMesaFrameBuffer)gl_fb)


/* AROS visual */
struct arosmesa_visual
{
    GLvisual            Base;                       /* Base class - must be first */
    enum pipe_format    ColorFormat;
    enum pipe_format    DepthFormat;
    enum pipe_format    StencilFormat;
};

typedef struct arosmesa_visual * AROSMesaVisual;

#define GET_GL_VIS_PTR(arosmesa_vis) (&arosmesa_vis->Base)
#define GET_AROS_VIS_PTR(gl_vis) ((AROSMesaVisual)gl_vis)

struct arosmesa_screen_info
{
    struct Screen * Screen;                         /* Current screen*/
    GLuint          Width;
    GLuint          Height;
    GLuint          Depth;
    GLuint          BitsPerPixel;
};

/* FIXME: Maybe the screen_surface should be hidden away in some framebuffer? */
struct pipe_surface;

/* AROS context */
struct arosmesa_context
{
    struct st_context *         st;                     /* Base class - must be first */
    AROSMesaVisual              visual;                 /* the visual context */
    AROSMesaFrameBuffer         framebuffer;
    
    /* FIXME: shouldn't this be part of frame buffer? */
    struct Window               *window;                /* Intuition window */
    struct arosmesa_screen_info ScreenInfo;
    struct pipe_screen          *pscreen;
    
    /* Rastport 'visible' to user (window rasport, screen rastport)*/
    struct RastPort             *visible_rp;
    /* Rastport dimentions */
    GLuint                      visible_rp_width;       /* the rastport drawing area full size*/
    GLuint                      visible_rp_height;      /* the rastport drawing area full size*/

    /* Buffer information */
    GLuint                      width, height;          /* drawable area on rastport defined by borders */
    GLuint                      top, bottom;            /* offsets due to window border */
    GLuint                      left, right;            /* offsets due to window border */    
};

#define GET_GL_CTX_PTR(arosmesa_ctx) (arosmesa_ctx->st->ctx)
#define GET_AROS_CTX_PTR(gl_ctx) ((AROSMesaContext)gl_ctx->DriverCtx)

#endif /* AROSMESA_INTERNAL_H */
