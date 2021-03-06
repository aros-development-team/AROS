/*
    Copyright 2011-2017, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "hostgl_ctx_manager.h"
#include "hostgl_funcs.h"
#include "hostgl_support.h"

#include <x11gfx_bitmapclass.h>

/*****************************************************************************

    NAME */

      void glADestroyContext(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Destroys the GL rendering context and frees all resoureces.
 
    INPUTS
        ctx - pointer to GL rendering context. A NULL pointer will be
                ignored.
 
    RESULT
        The GL context is destroyed. Do no use it anymore.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct hostgl_context *_ctx = (struct hostgl_context *)ctx;

    if (_ctx)
    {
        Display * dsp = NULL;
        struct hostgl_context *cur_ctx = NULL;

        HostGL_Lock();

        dsp = HostGL_GetGlobalX11Display();
        cur_ctx = HostGL_GetCurrentContext();

        /* If the passed context is current context, detach it */
        if (_ctx == cur_ctx)
        {
            HostGL_SetCurrentContext(NULL);
            HostGL_UpdateGlobalGLXContext();
        }

        if (_ctx->glXctx)
            GLXCALL(glXDestroyContext, dsp, _ctx->glXctx);

#if defined(RENDERER_SEPARATE_X_WINDOW)
        GLXCALL(glXDestroyWindow, dsp, _ctx->glXWindow);
        XCALL(XDestroyWindow, dsp, _ctx->XWindow);
#endif
#if defined(RENDERER_PBUFFER_WPA)
        HostGL_DeAllocatePBuffer(_ctx);
#endif
#if defined(RENDERER_PIXMAP_BLIT)
        HostGL_DeAllocatePixmap(_ctx);
#endif

        if (_ctx->HiddX11BitMapAB)
            OOP_ReleaseAttrBase(IID_Hidd_BitMap_X11);

        XCALL(XFree, _ctx->framebuffer->fbconfigs);
        FreeVec(_ctx->framebuffer);
        HostGLFreeContext(ctx);

        HostGL_UnLock();
    }
}

