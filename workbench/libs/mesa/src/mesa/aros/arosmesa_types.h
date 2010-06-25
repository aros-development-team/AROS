/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_TYPES_H
#define AROSMESA_TYPES_H

#include "main/mtypes.h"
#include "state_tracker/st_context.h"

#include <GL/arosmesa.h>

typedef struct st_framebuffer * AROSMesaFrameBuffer;

/* AROSMesa visual */
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

/* AROSMesa context */
struct arosmesa_context
{
    struct st_context           *st;
    AROSMesaFrameBuffer         framebuffer;
    AROSMesaVisual              visual;                 /* visual context */
    struct pipe_screen          *pscreen;

    struct Window               *window;                /* Intuition window */
    struct arosmesa_screen_info ScreenInfo;
    
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

#endif /* AROSMESA_INTERNAL_H */
