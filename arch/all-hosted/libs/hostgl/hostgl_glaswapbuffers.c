/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
*/

#include "hostgl_ctx_manager.h"
#include "hostgl_funcs.h"
#include <proto/exec.h>
#include <aros/debug.h>
#if defined(RENDERER_PBUFFER_WPA)
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <cybergraphx/cybergraphics.h>
static struct SignalSemaphore * GetX11SemaphoreFromBitmap(struct BitMap * bm);
#endif
#if defined(RENDERER_PIXMAP_BLIT)
#include <proto/graphics.h>
#endif
/*****************************************************************************

    NAME */

      void glASwapBuffers(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Swaps the back with front buffers. MUST BE used to display the effect
        of rendering onto the target RastPort, since GLA  always work in
        double buffer mode.
 
    INPUTS
        ctx - GL rendering context on which swap is to be performed.
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct hostgl_context *_ctx = (struct hostgl_context *)ctx;

    D(bug("[HostGL] TASK: 0x%x, SWAP 0x%x\n", FindTask(NULL), _ctx->glXctx));

    if (_ctx)
    {
#if defined(RENDERER_SEPARATE_X_WINDOW)
        Display * dsp = NULL;
        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        dsp = HostGL_GetGlobalX11Display();
        GLXCALL(glXSwapBuffers, dsp, _ctx->glXWindow);
        HostGL_UnLock();
#endif
#if defined(RENDERER_PBUFFER_WPA)
        LONG line = 0;
        LONG width = _ctx->framebuffer->width;
        LONG height = _ctx->framebuffer->height;
        
        /* This codes works correct with both 16bpp and 32 bpp pixel buffers */

        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        ObtainSemaphore(GetX11SemaphoreFromBitmap(_ctx->visible_rp->BitMap));
        GLCALL(glReadPixels, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, _ctx->swapbuffer);
        ReleaseSemaphore(GetX11SemaphoreFromBitmap(_ctx->visible_rp->BitMap));
        HostGL_UnLock();

        /* Flip image */
        for (line = 0; line < height / 2; line++)
        {
            ULONG * start = _ctx->swapbuffer + (line * width);
            ULONG * end = _ctx->swapbuffer + ((height - line - 1) * width);
            CopyMem(start, _ctx->swapbufferline, width * SWAPBUFFER_BPP);
            CopyMem(end, start, width * SWAPBUFFER_BPP);
            CopyMem(_ctx->swapbufferline, end, width * SWAPBUFFER_BPP);
        }

        WritePixelArray(_ctx->swapbuffer, 0, 0, width * SWAPBUFFER_BPP, _ctx->visible_rp, _ctx->left, _ctx->top,
            width, height, RECTFMT_BGRA32);
#endif
#if defined(RENDERER_PIXMAP_BLIT)
        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        GLXCALL(glXWaitGL);
        HostGL_UnLock();

        BltBitMapRastPort(_ctx->glXPixmapBM, 0, 0,
                          _ctx->visible_rp, _ctx->left, _ctx->top,
                          _ctx->framebuffer->width, _ctx->framebuffer->height,
                          192);

        HostGL_Lock();
        HostGL_UpdateGlobalGLXContext();
        GLXCALL(glXWaitX);
        HostGL_UnLock();
#endif
        HostGL_CheckAndUpdateBufferSize(_ctx);
    }
}

#if defined(RENDERER_PBUFFER_WPA)
/* HACK: EVIL HACK TO GET ACCESS TO X11 HIDD INTERNALS */
#include <proto/oop.h>
#define HIDD_BM_OBJ(bitmap)     (*(OOP_Object **)&((bitmap)->Planes[0]))

struct bitmap_data
{
    union
    {
        Window  xwindow;
        Pixmap  pixmap;
    }               drawable;
    Window          masterxwindow;
    Cursor          cursor;
    unsigned long   sysplanemask;
    Colormap        colmap;
    GC              gc; /* !!! This is an X11 GC, NOT a HIDD gc */
    Display         *display;
    int             screen;
    int             flags;
};

struct fakefb_data
{
    OOP_Object *framebuffer;
    OOP_Object *fakegfxhidd;
};

struct x11_staticdata
{
    /* These two members MUST be in the beginning of this structure
       because they are exposed to disk-based part (see x11_class.h) */
    UBYTE                    keycode2rawkey[256];
    BOOL                     havetable;

    struct SignalSemaphore   sema; /* Protecting this whole struct */
    struct SignalSemaphore   x11sema;
};

struct x11clbase
{
    struct Library        library;
    
    struct x11_staticdata xsd;
};

#define XSD(cl)         (&((struct x11clbase *)cl->UserData)->xsd)

static struct SignalSemaphore * GetX11SemaphoreFromBitmap(struct BitMap * bm)
{
    static struct SignalSemaphore * x11sem = NULL;

    if (x11sem == NULL)
    {
        OOP_Object * hiddbm = HIDD_BM_OBJ(bm);
        struct fakefb_data * fakefb = (struct fakefb_data *)OOP_INST_DATA(OOP_OCLASS(hiddbm), hiddbm);
        x11sem = (&XSD(OOP_OCLASS(fakefb->framebuffer))->x11sema);
    }

    return x11sem;
}
#endif
