/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa_internal.h 31775 2009-09-06 16:49:16Z deadwood $
*/

#if !defined(AROSMESA_INTERNAL_H)
#define AROSMESA_INTERNAL_H

#include "mtypes.h"
#include <GL/arosmesa.h>


extern struct Library * AROSMesaCyberGfxBase;
#define CyberGfxBase AROSMesaCyberGfxBase

/* TrueColor (RGBA) to ColorFrmt Macros */
#define     TC_ARGB(r, g, b, a)     ((((((a << 8) | r) << 8) | g) << 8) | b)
#define     TC_ARGB_ARR(array)      ((((((array[ACOMP] << 8) | array[RCOMP]) << 8) | array[GCOMP]) << 8) | array[BCOMP])
#define     TC_RGBA(r, g, b, a)     ((((((r << 8) | g) << 8) | b) << 8) | a)
#define     PACK_8B8G8R( r, g, b )  (((((b << 8) | g) << 8) | r) << 8)

/*
 * Convert Mesa window coordinate(s) to AROS screen coordinate(s):
 */

#define FIXx(x) (amesa->left + (x))

#define FIXy(y) (amesa->top + (amesa->height - 1) - (y))

#define CorrectY(y) (rb->Height - 1 - (y))


/* Structs */



/* AROS render buffer */
struct arosmesa_renderbuffer
{
    struct gl_renderbuffer      Base;               /* Base class - must be first */
    UBYTE *                     buffer;             /* Draw buffer in ABGR format */
};

typedef struct arosmesa_renderbuffer * AROSMesaRenderBuffer;

#define GET_GL_RB_PTR(arosmesa_rb) (&arosmesa_rb->Base)
#define GET_AROS_RB_PTR(gl_rb) ((AROSMesaRenderBuffer)gl_rb)

/* AROS frame buffer */
struct arosmesa_framebuffer
{
    struct gl_framebuffer Base;     /* Base class - must be first */
};

typedef struct arosmesa_framebuffer * AROSMesaFrameBuffer;

#define GET_GL_FB_PTR(arosmesa_fb) (&arosmesa_fb->Base)
#define GET_AROS_FB_PTR(gl_fb) ((AROSMesaFrameBuffer)gl_fb)

/* AROS visual */
struct arosmesa_visual
{
    GLvisual Base;                  /* Base class - must be first */
};

typedef struct arosmesa_visual * AROSMesaVisual;

#define GET_GL_VIS_PTR(arosmesa_vis) (&arosmesa_vis->Base)
#define GET_AROS_VIS_PTR(gl_vis) ((AROSMesaVisual)gl_vis)

/* AROS context */

struct arosmesa_context
{
    GLcontext               Base;                   /* GLcontext* - the core library context */

    AROSMesaVisual          visual;                 /* the visual context */
    AROSMesaFrameBuffer     framebuffer;            /* frame buffer */
    AROSMesaRenderBuffer    renderbuffer;           /* render buffer (can be considered back buffer) */

    ULONG                   clearpixel;             /* pixel for clearing the color buffers */

    struct Window           *window;                /* Intuition window */
    struct Screen           *screen;                /* Current screen*/
    
    /* Rastport 'visible' to user (window rasport, screen rastport)*/
    struct RastPort         *visible_rp;            
    /* Rastport dimentions */
    GLuint                  visible_rp_width;       /* the rastport drawing area full size*/
    GLuint                  visible_rp_height;      /* the rastport drawing area full size*/

    /* Buffer information */
    GLuint                  depth;                  /* bits per pixel (1, 8, 24, etc) */
    GLuint                  width, height;          /* drawable area on rastport defined by borders */
    GLuint                  top, bottom;            /* offsets due to window border */
    GLuint                  left, right;            /* offsets due to window border */
};

/* typedef struct arosmesa_context * AROSMesaContext; */ /* Defined in AROSMesa.h */

#define GET_GL_CTX_PTR(arosmesa_ctx) (&arosmesa_ctx->Base)
#define GET_AROS_CTX_PTR(gl_ctx) ((AROSMesaContext)gl_ctx) /* FIXME: Should be gl_ctx->DriverCtx? */

#endif /* AROSMESA_INTERN_H */
