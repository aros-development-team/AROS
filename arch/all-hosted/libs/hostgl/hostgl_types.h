/*
    Copyright 2011-2025, The AROS Development Team. All rights reserved.
*/

#ifndef HOSTGL_TYPES_H
#define HOSTGL_TYPES_H

#include <oop/oop.h>
#include <GL/gla.h>

#include "hostgl_renderer_config.h"

#include "x11_hostlib.h"
#include "glx_hostlib.h"

struct hostgl_framebuffer
{
    union {
        GLXFBConfig             *fbconfigs;
        struct glx_config       **glxconfigs;
    };
    ULONG                       width;
    ULONG                       height;
    BOOL                        resized;
};

struct hostgl_context
{
    OOP_AttrBase HiddX11BitMapAB;
    XVisualInfo     *visinfo;
#if defined(RENDERER_SEPARATE_X_WINDOW)
    Window      XWindow;
    GLXWindow   glXWindow;
#endif
#if defined(RENDERER_BUFFER)
    union {
        GLXPixmap       glXPixmap;
        GLXPbuffer      glXPbuffer;
    };
    union {
        struct BitMap   *glXPixmapBM;
        ULONG           *swapbuffer;
    };
    ULONG           *swapbufferline;
#endif
    GLXContext  glXctx;

    struct hostgl_framebuffer *framebuffer;
    struct Window               *window;
    struct Screen               *Screen;
    ULONG                       BitsPerPixel;
    struct RastPort             *visible_rp;
    ULONG                       visible_rp_width;
    ULONG                       visible_rp_height;
    ULONG                      top, bottom;
    ULONG                      left, right;
};

#define SWAPBUFFER_BPP  (4) /* Swap buffer always has 4 bytes per pixel, even on 16bpp screens */

#endif /* HOSTGL_TYPES_H */
