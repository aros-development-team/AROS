/*
    Copyright (C) 2011-2025, The AROS Development Team. All rights reserved.
*/

#define AROS_TAGRETURNTYPE  GLAContext

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "hostgl_ctx_manager.h"
#include "hostgl_funcs.h"
#include "hostgl_support.h"

#include <x11gfx_bitmapclass.h>

#define HOSTGL_SAFESTACK    (1 << 19)

/*****************************************************************************

    NAME */

      GLAContext glACreateContext(

/*  SYNOPSIS */
      struct TagItem *tagList)

/*  FUNCTION

        Crates a GL rendering context. Whether the rendering will be software
        or hardware based depends on the gallium.library returning a module
        best suited.
 
    INPUTS

        tagList - a pointer to tags to be used during creation.
 
    TAGS

        GLA_Left   - specifies the left rendering offset on the rastport.
                     Typically equals to window->BorderLeft.

        GLA_Top    - specifies the top rendering offset on the rastport.
                     Typically equals to window->BorderTop.

        GLA_Right  - specifies the right rendering offset on the rastport.
                     Typically equals to window->BorderRight.

        GLA_Bottom - specifies the bottom rendering offset on the rastport.
                     Typically equals to window->BorderBottom.
    
        GLA_Width  - specifies the width of the rendering area.
                     GLA_Width + GLA_Left + GLA_Right should equal the width of
                     the rastport. The GLA_Width is interchangable at cration
                     time with GLA_Right. Later durring window resizing, width
                     is calculated from scalled left, righ and window width.

        GLA_Height - specifies the height of the rendering area.
                     GLA_Height + GLA_Top + GLA_Bottom should equal the height
                     of the rastport. The GLA_Height is interchangable at
                     cration time with GLA_Bottom. Later durring window resizing
                     , height is calculated from scalled top, bottom and window
                     height.

        GLA_Screen - pointer to Screen onto which scene is to be rendered. When
                     selecting RastPort has lower priority than GLA_Window.

        GLA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

        GLA_RastPort - ignored. Use GLA_Window.

        GLA_DoubleBuf - ignored. All rendering is always double buffered.

        GLA_RGBMode - ignored. All rendering is done in RGB. Indexed modes are
                      not supported.

        GLA_AlphaFlag - ignored. All rendering is done with alpha channel.

        GLA_NoDepth - disables the depth/Z buffer. Depth buffer is enabled by
                      default and is 16 or 24 bit based on rendering
                      capabilities.

        GLA_NoStencil - disables the stencil buffer. Stencil buffer is enabled
                        by default.

        GLA_NoAccum - disables the accumulation buffer. Accumulation buffer is
                      enabled by default.

    RESULT

        A valid GL context or NULL of creation was not successful.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    LONG                    screen;
    struct hostgl_context *ctx = NULL;
    LONG                    numreturned;
    Display                 *dsp = NULL;
    const LONG              fbattributessize = 40;
    LONG                    fbattributes[fbattributessize];
#if defined(RENDERER_SEPARATE_X_WINDOW)
    XSetWindowAttributes    swa;
    LONG                    swamask;
#endif

    D(
        struct Task *ctx_task = FindTask(NULL);
        bug("[HostGL] %s()\n", __func__);
        //Sanity check
        if (((IPTR)ctx_task->tc_SPUpper - (IPTR)ctx_task->tc_SPLower) < HOSTGL_SAFESTACK) {
            bug("[HostGL] %s: called with very low stack - may crash ( < %ubytes)\n", __func__, HOSTGL_SAFESTACK);
        }
    )

    HostGL_Lock();

    /* Standard glA initialization */

    /* Allocate HostGL context struct initialized to zeros */
    if (!(ctx = (struct hostgl_context *)AllocVec(sizeof(struct hostgl_context), MEMF_PUBLIC | MEMF_CLEAR))) {
        bug("%s: ERROR - failed to allocate Context\n", __func__);
        goto error_out;
    }

    D(bug("[HostGL] %s: ctx @ 0x%p\n", __func__, ctx));
    
    ctx->HiddX11BitMapAB = OOP_ObtainAttrBase(IID_Hidd_BitMap_X11);

    HostGLSelectRastPort(ctx, tagList);
    if (!ctx->visible_rp) {
        bug("%s: ERROR - failed to select visible rastport\n", __func__);
        goto error_out;
    }

    HostGLStandardInit(ctx, tagList);

    D(bug("[HostGL] %s: allocating framebuffer ..\n", __func__));

    ctx->framebuffer = (struct hostgl_framebuffer *)AllocVec(sizeof(struct hostgl_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);
    if (!ctx->framebuffer) {
        bug("%s: ERROR -  failed to create frame buffer\n", __func__);
        goto error_out;
    }

    HostGLRecalculateBufferWidthHeight(ctx);

    /* X/GLX initialization */
    D(bug("[HostGL] %s: initializing Host ..\n", __func__));
    
    /* Get connection with the server */
    dsp = HostGL_GetGlobalX11Display();
    D(bug("[HostGL] %s: dsp @ 0x%p\n", __func__, dsp));
    screen = DefaultScreen(dsp);
    D(bug("[HostGL] %s: screen @ 0x%p\n", __func__, screen));

    int novisinfo = 0;
#if !defined(RENDERER_SEPARATE_X_WINDOW)
retryconfigs:
#endif
    if (!HostGL_FillFBAttributes(fbattributes, fbattributessize, tagList, novisinfo)) {
        bug("%s: ERROR -  failed to fill FB attributes\n", __func__);
        goto error_out;
    }

    /* Choose fb config */
    ctx->framebuffer->fbconfigs = GLXCALL(glXChooseFBConfig, dsp, screen, fbattributes, &numreturned);
    if (ctx->framebuffer->fbconfigs == NULL) {
        bug("%s: ERROR -  failed to retrieve fbconfigs\n", __func__);
        goto error_out;
    }
    D(bug("[HostGL] %s: fbconfigs @ 0x%p\n", __func__, ctx->framebuffer->fbconfigs));

    if (novisinfo == 0)
        ctx->visinfo = GLXCALL(glXGetVisualFromFBConfig, dsp, ctx->framebuffer->fbconfigs[0]);
#if !defined(RENDERER_SEPARATE_X_WINDOW)
    if ((novisinfo == 0) && (ctx->visinfo == NULL)) {
        XCALL(XFree, ctx->framebuffer->fbconfigs);
        ctx->framebuffer->fbconfigs = NULL;
        novisinfo = 1;
        goto retryconfigs;
    }
#else
    D(bug("[HostGL] %s: initializing X ..\n", __func__));
    swa.colormap = XCALL(XCreateColormap, dsp, RootWindow(dsp, screen), ctx->visinfo->visual, AllocNone);
    swamask = CWColormap;

    /* Create X window */
    ctx->XWindow = XCALL(XCreateWindow, dsp, RootWindow(dsp, screen),
        ctx->left, ctx->top, ctx->framebuffer>width, ctx->framebuffer>height, 0,
        ctx->visinfo->depth, InputOutput, ctx->visinfo->visual, swamask, &swa);
    
    /* Create GLX window */
    ctx->glXWindow = GLXCALL(glXCreateWindow, dsp, ctx->framebuffer>fbconfigs[0], ctx->XWindow, NULL);

    /* Map (show) the window */
    XCALL(XMapWindow, dsp, ctx->XWindow);
    
    XCALL(XFlush, dsp);

    /* Create GL context */
    ctx->glXctx = GLXCALL(glXCreateNewContext, dsp, ctx->framebuffer>fbconfigs[0], GLX_RGBA_TYPE, NULL, True);

#endif

#if defined(RENDERER_BUFFER)
    D(bug("[HostGL] %s: initializing buffer ..\n", __func__));
    HostGL_AllocateBuffer(ctx);
    D(bug("[HostGL] %s: creating context ..\n", __func__));
    ctx->glXctx = GLXCALL(glXCreateNewContext, dsp, ctx->framebuffer->fbconfigs[0], GLX_RGBA_TYPE, NULL, True);
#endif

    if (!ctx->glXctx) {
        bug("%s: ERROR -  failed to create GLX context\n", __func__);
        goto error_out;
    }

    D(bug("[HostGL] %s: ThisTask @ 0x%p, glXctx @ 0x%p\n", __func__, ctx_task, ctx->glXctx));

    HostGL_UnLock();

    D(bug("[HostGL] %s: returning 0x%p\n", __func__, ctx));

    return (GLAContext)ctx;

error_out:
#if defined(RENDERER_SEPARATE_X_WINDOW)
    if (ctx && ctx->glXWindow) GLXCALL(glXDestroyWindow, dsp, ctx->glXWindow);
    if (ctx && ctx->XWindow) XCALL(XDestroyWindow, dsp, ctx->XWindow);
#endif
#if defined(RENDERER_BUFFER)
    if (ctx) HostGL_DeAllocateBuffer(ctx);
#endif

    if (ctx->HiddX11BitMapAB)
        OOP_ReleaseAttrBase(IID_Hidd_BitMap_X11);

    if (ctx->framebuffer) {
        if (ctx->framebuffer->fbconfigs) XCALL(XFree, ctx->framebuffer->fbconfigs);
        FreeVec(ctx->framebuffer);
    }
    if (ctx) HostGLFreeContext(ctx);

    HostGL_UnLock();
    return (GLAContext)NULL;
}

