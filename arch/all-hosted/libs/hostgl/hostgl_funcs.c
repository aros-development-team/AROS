/*
    Copyright 2011-2017, The AROS Development Team. All rights reserved.
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include <hidd/gfx.h>

#include "hostgl_ctx_manager.h"
#include "hostgl_funcs.h"
#include "hostgl_support.h"

#include <x11gfx_bitmapclass.h>

#define SETFBATTR(attribute, value)     \
    {                                   \
        fbattributes[i++] = attribute;  \
        fbattributes[i++] = value;      \
    }

BOOL HostGL_FillFBAttributes(LONG * fbattributes, LONG size, struct TagItem *tagList)
{
    LONG i = 0;
    BOOL noDepth, noStencil, noAccum;
    noStencil   = GetTagData(GLA_NoStencil, GL_FALSE, tagList);
    noAccum     = GetTagData(GLA_NoAccum, GL_FALSE, tagList);
    noDepth     = GetTagData(GLA_NoDepth, GL_FALSE, tagList);

#if defined(RENDERER_SEPARATE_X_WINDOW)
    SETFBATTR(GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT)
    SETFBATTR(GLX_DOUBLEBUFFER, True);
#endif

#if defined(RENDERER_PBUFFER_WPA)
    SETFBATTR(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT)
    SETFBATTR(GLX_DOUBLEBUFFER, False);
#endif

#if defined(RENDERER_PIXMAP_BLIT)
    SETFBATTR(GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT)
    SETFBATTR(GLX_DOUBLEBUFFER, False);
#endif

    SETFBATTR(GLX_RENDER_TYPE, GLX_RGBA_BIT)
    SETFBATTR(GLX_RED_SIZE, 1)
    SETFBATTR(GLX_GREEN_SIZE, 1)
    SETFBATTR(GLX_BLUE_SIZE, 1)
    SETFBATTR(GLX_ALPHA_SIZE, 0)

    if (noDepth)
        SETFBATTR(GLX_DEPTH_SIZE, 0)
    else
        SETFBATTR(GLX_DEPTH_SIZE, 24)

    if (noStencil)
        SETFBATTR(GLX_STENCIL_SIZE, 0)
    else
        SETFBATTR(GLX_STENCIL_SIZE, 8)

    if (noAccum)
    {
        SETFBATTR(GLX_ACCUM_RED_SIZE, 0)
        SETFBATTR(GLX_ACCUM_GREEN_SIZE, 0)
        SETFBATTR(GLX_ACCUM_BLUE_SIZE, 0)
        SETFBATTR(GLX_ACCUM_ALPHA_SIZE, 0)
    }
    else
    {
        SETFBATTR(GLX_ACCUM_RED_SIZE, 16)
        SETFBATTR(GLX_ACCUM_GREEN_SIZE, 16)
        SETFBATTR(GLX_ACCUM_BLUE_SIZE, 16)
        SETFBATTR(GLX_ACCUM_ALPHA_SIZE, 16)
    }

    SETFBATTR(None, None)

    return TRUE;
}

#if defined(RENDERER_PBUFFER_WPA)
/* This function assumes all storages are NULL and/or have been freed */
VOID HostGL_AllocatePBuffer(struct hostgl_context *ctx)
{
    Display * dsp = HostGL_GetGlobalX11Display();
    LONG pbufferattributes[] =
    {
        GLX_PBUFFER_WIDTH,   0,
        GLX_PBUFFER_HEIGHT,  0,
        GLX_LARGEST_PBUFFER, False,
        None
    };

    pbufferattributes[1] = ctx->framebuffer->width;
    pbufferattributes[3] = ctx->framebuffer->height;
    ctx->glXPbuffer = GLXCALL(glXCreatePbuffer, dsp, ctx->framebuffer->fbconfigs[0], pbufferattributes);

    ctx->swapbuffer       = AllocVec(ctx->framebuffer->width * ctx->framebuffer->height * SWAPBUFFER_BPP, MEMF_ANY);
    ctx->swapbufferline   = AllocVec(ctx->framebuffer->width * SWAPBUFFER_BPP, MEMF_ANY);
}

VOID HostGL_DeAllocatePBuffer(struct hostgl_context *ctx)
{
    Display * dsp = HostGL_GetGlobalX11Display();

    if (ctx->glXPbuffer) GLXCALL(glXDestroyPbuffer, dsp, ctx->glXPbuffer);
    if (ctx->swapbufferline) FreeVec(ctx->swapbufferline);
    if (ctx->swapbuffer) FreeVec(ctx->swapbuffer);

    ctx->glXPbuffer = None;
    ctx->swapbufferline = NULL;
    ctx->swapbuffer = NULL;
}

static VOID HostGL_ResizePBuffer(struct hostgl_context *ctx) /* Must be called with lock held */
{
    struct hostgl_context *cur_ctx = HostGL_GetCurrentContext();
    BOOL isCurrent = (cur_ctx == ctx);
    Display * dsp = HostGL_GetGlobalX11Display();

    /* If current, detach */
    if (isCurrent)
        GLXCALL(glXMakeContextCurrent, dsp, None, None, NULL);

    /* Destroy and recreate (using new size) */
    HostGL_DeAllocatePBuffer(ctx);
    HostGL_AllocatePBuffer(ctx);

    /* If current, attach */
    if (isCurrent)
        GLXCALL(glXMakeContextCurrent, dsp, cur_ctx->glXPbuffer, cur_ctx->glXPbuffer, cur_ctx->glXctx);

    ctx->framebuffer->resized = FALSE;
}
#endif

#if defined(RENDERER_PIXMAP_BLIT)
/* This function assumes all storages are NULL and/or have been freed */
VOID HostGL_AllocatePixmap(struct hostgl_context *ctx)
{
    Display     *dsp = HostGL_GetGlobalX11Display();
    OOP_Object  *hiddbm;
    Pixmap       pixmap = (Pixmap)0;

    ctx->glXPixmapBM = AllocBitMap(ctx->framebuffer->width, ctx->framebuffer->height, 0, BMF_MINPLANES, ctx->visible_rp->BitMap);

#define HiddX11BitMapAB ctx->HiddX11BitMapAB
    
    if(ctx->glXPixmapBM)
    {
        if ((hiddbm = HIDD_BM_OBJ(ctx->glXPixmapBM)))
        {
            OOP_GetAttr(hiddbm, aHidd_BitMap_X11_Drawable, (IPTR *) &pixmap);

            ctx->glXPixmap = GLXCALL(glXCreateGLXPixmap, dsp, ctx->visinfo, pixmap);
        }
    }
#undef HiddX11BitMapAB
}

VOID HostGL_DeAllocatePixmap(struct hostgl_context *ctx)
{
    Display * dsp = HostGL_GetGlobalX11Display();

    if (ctx->glXPixmap) GLXCALL(glXDestroyGLXPixmap, dsp, ctx->glXPixmap);
    if (ctx->glXPixmapBM) FreeBitMap(ctx->glXPixmapBM);
    
    ctx->glXPixmap = None;
    ctx->glXPixmapBM = NULL;
}

static VOID HostGL_ResizePixmap(struct hostgl_context * ctx) /* Must be called with lock held */
{
    struct hostgl_context *cur_ctx = HostGL_GetCurrentContext();
    BOOL isCurrent = (cur_ctx == ctx);
    Display * dsp = HostGL_GetGlobalX11Display();

    /* If current, detach */
    if (isCurrent)
        GLXCALL(glXMakeContextCurrent, dsp, None, None, NULL);

    /* Destroy and recreate (using new size) */
    HostGL_DeAllocatePixmap(ctx);
    HostGL_AllocatePixmap(ctx);

    /* If current, attach */
    if (isCurrent)
        GLXCALL(glXMakeContextCurrent, dsp, cur_ctx->glXPixmap, cur_ctx->glXPixmap, cur_ctx->glXctx);

    ctx->framebuffer->resized = FALSE;
}
#endif

VOID HostGL_CheckAndUpdateBufferSize(struct hostgl_context * ctx)
{
    HostGLRecalculateBufferWidthHeight(ctx);
#if defined(RENDERER_PBUFFER_WPA)
    if (ctx->framebuffer->resized)
    {
        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        HostGL_ResizePBuffer(ctx);
        HostGL_UnLock();
    }
#endif
#if defined(RENDERER_PIXMAP_BLIT)
    if (ctx->framebuffer->resized)
    {
        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        HostGL_ResizePixmap(ctx);
        HostGL_UnLock();
    }
#endif
}
