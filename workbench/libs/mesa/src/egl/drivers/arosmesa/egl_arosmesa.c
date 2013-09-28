/*
    Copyright 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "eglsurface.h"
#include "eglcontext.h"
#include "eglconfig.h"
#include "egldisplay.h"
#include "egldriver.h"

#include <stdlib.h>

#include <GL/arosmesa.h>

extern struct Library * MesaBase;

/* General note: due to fact that AROSMesa API is not symmetric with EGL api,
 * AROSMesa context is not created in EGL create context, but in EGL MakeCurrent
 */

struct egl_arosmesa
{
    _EGLDriver base;
};

struct egl_arosmesa_context
{
    _EGLContext base;
    AROSMesaContext amesactx;
};

static inline struct egl_arosmesa_context * egl_arosmesa_context(_EGLContext * ctx)
{
    return (struct egl_arosmesa_context *)ctx;
}

struct egl_arosmesa_surface
{
    _EGLSurface base;
    struct Window * win;
};

static _EGLProc egl_arosmesa_getprocaddress(_EGLDriver *drv, const char *procname)
{
    (void) drv;

    return AROSMesaGetProcAddress(procname);
}

static EGLBoolean egl_arosmesa_terminate(_EGLDriver *drv, _EGLDisplay *disp)
{
    _eglReleaseDisplayResources(drv, disp);
    _eglCleanupDisplay(disp);

    disp->DriverData = NULL;

    return EGL_TRUE;
}

static EGLBoolean egl_arosmesa_destroycontext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
    (void) drv;

    if (_eglPutContext(ctx))
    {
        struct egl_arosmesa_context * eglctx = egl_arosmesa_context(ctx);
        AROSMesaMakeCurrent(NULL);
        if (eglctx->amesactx)
            AROSMesaDestroyContext(eglctx->amesactx);
        free(eglctx);
    }

    return EGL_TRUE;
}

static EGLBoolean egl_arosmesa_destroysurface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
    (void) drv;

    if (_eglPutSurface(surf))
    {
        free(surf);
    }

    return EGL_TRUE;
}

static EGLBoolean egl_arosmesa_swapbuffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
    AROSMesaSwapBuffers(disp->DriverData);
    return EGL_TRUE;
}

static EGLBoolean egl_arosmesa_makecurrent(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
        _EGLSurface *rsurf, _EGLContext *ctx)
{
    _EGLContext *old_ctx;
    _EGLSurface *old_dsurf, *old_rsurf;

    if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
        return EGL_FALSE;

    /* Since AROSMesa API is not symmetric with EGL, context handling needs to be performed here */

    /* Do nothing */
    if ((ctx == NULL) && (old_ctx == NULL))
        return EGL_TRUE;

    /* Unbind the current context */
    if ((ctx == NULL) && (old_ctx != NULL))
    {
        AROSMesaMakeCurrent(NULL);
        _eglPutSurface(old_dsurf);
        _eglPutSurface(old_rsurf);
        _eglPutContext(old_ctx);
        disp->DriverData = NULL;
        return EGL_TRUE;
    }

    /* Create when needed and bind new context */
    if (ctx != NULL)
    {
        struct egl_arosmesa_context * eglctx = egl_arosmesa_context(ctx);

        if (old_ctx != NULL)
        {
            _eglPutSurface(old_dsurf);
            _eglPutSurface(old_rsurf);
            _eglPutContext(old_ctx);
            disp->DriverData = NULL;
        }

        if (eglctx->amesactx == NULL)
        {
            struct TagItem attributes [14];
            struct Window * win = ((struct egl_arosmesa_surface *)dsurf)->win;
            int i = 0;

            attributes[i].ti_Tag = AMA_Window;      attributes[i++].ti_Data = (IPTR)win;
            attributes[i].ti_Tag = AMA_Left;        attributes[i++].ti_Data = win->BorderLeft;
            attributes[i].ti_Tag = AMA_Top;         attributes[i++].ti_Data = win->BorderTop;
            attributes[i].ti_Tag = AMA_Bottom;      attributes[i++].ti_Data = win->BorderBottom;
            attributes[i].ti_Tag = AMA_Right;       attributes[i++].ti_Data = win->BorderRight;

            attributes[i].ti_Tag = AMA_DoubleBuf;   attributes[i++].ti_Data = GL_TRUE;

            attributes[i].ti_Tag = AMA_RGBMode;     attributes[i++].ti_Data = GL_TRUE;

            attributes[i].ti_Tag = AMA_NoStencil;   attributes[i++].ti_Data = GL_TRUE;
            attributes[i].ti_Tag = AMA_NoAccum;     attributes[i++].ti_Data = GL_TRUE;

            attributes[i].ti_Tag = TAG_DONE;

            eglctx->amesactx = AROSMesaCreateContext(attributes);
        }

        if (eglctx->amesactx != NULL)
        {
            AROSMesaMakeCurrent(eglctx->amesactx);
            disp->DriverData = eglctx->amesactx;
            return EGL_TRUE;
        }
    }

    return EGL_FALSE;
}

static _EGLSurface * egl_arosmesa_createwindowsurface(_EGLDriver *drv, _EGLDisplay *disp,
        _EGLConfig *conf, EGLNativeWindowType window, const EGLint *attrib_list)
{
    struct egl_arosmesa_surface * surf = calloc(1, sizeof(struct egl_arosmesa_surface));

    if (!_eglInitSurface(&surf->base, disp, EGL_WINDOW_BIT, conf, attrib_list))
    {
        free(surf);
        return NULL;
    }

    surf->base.Width = window->Width - window->BorderLeft - window->BorderRight;
    surf->base.Height = window->Height - window->BorderTop -  window->BorderBottom;
    surf->win = window;

    return &surf->base;
}

static _EGLContext * egl_arosmesa_createcontext(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
        _EGLContext *share_list, const EGLint *attrib_list)
{
    struct egl_arosmesa_context * ctx = calloc(1, sizeof(struct egl_arosmesa_context));

    if (!_eglInitContext(&ctx->base, disp, conf, attrib_list))
    {
        free(ctx);
        return NULL;
    }

    ctx->amesactx = NULL; /* Context is created elsewhere */

    return &ctx->base;
}

static void create_configs(_EGLDisplay *dpy)
{
    _EGLConfig * cfg = calloc(1, sizeof(_EGLConfig));
    _eglInitConfig(cfg, dpy, 1);

    cfg->RenderableType = EGL_OPENGL_BIT;
    cfg->Conformant = EGL_OPENGL_BIT;
    cfg->SurfaceType = EGL_WINDOW_BIT;

    _eglSetConfigKey(cfg, EGL_RED_SIZE, 8);
    _eglSetConfigKey(cfg, EGL_GREEN_SIZE, 8);
    _eglSetConfigKey(cfg, EGL_BLUE_SIZE, 8);
    _eglSetConfigKey(cfg, EGL_ALPHA_SIZE, 8);
    _eglSetConfigKey(cfg, EGL_DEPTH_SIZE, 24);
    _eglSetConfigKey(cfg, EGL_STENCIL_SIZE, 8);

    _eglLinkConfig(cfg);
}

static EGLBoolean
egl_arosmesa_initialize(_EGLDriver *drv, _EGLDisplay *dpy)
{
    if (dpy->Options.TestOnly)
        return EGL_TRUE;

    create_configs(dpy);

    dpy->ClientAPIs |= EGL_OPENGL_BIT;
    dpy->VersionMajor = 1;
    dpy->VersionMinor = 4;

    return EGL_TRUE;
}

void egl_arosmesa_init_driver_api(_EGLDriver * drv)
{

    _eglInitDriverFallbacks(drv);

    drv->API.Initialize = egl_arosmesa_initialize;
    drv->API.CreateContext = egl_arosmesa_createcontext;
    drv->API.CreateWindowSurface = egl_arosmesa_createwindowsurface;
    drv->API.MakeCurrent = egl_arosmesa_makecurrent;
    drv->API.SwapBuffers = egl_arosmesa_swapbuffers;
    drv->API.DestroySurface = egl_arosmesa_destroysurface;
    drv->API.DestroyContext = egl_arosmesa_destroycontext;
    drv->API.Terminate = egl_arosmesa_terminate;
    drv->API.GetProcAddress = egl_arosmesa_getprocaddress;
}

void egl_arosmesa_unload(_EGLDriver *drv)
{
    free(drv);
}

_EGLDriver *
_eglBuiltInDriverAROSMesa(const char *args)
{
    struct egl_arosmesa * drv = calloc(1, sizeof(struct egl_arosmesa));

    if (!MesaBase)
        MesaBase = OpenLibrary("mesa.library", 0L);

    if (MesaBase)
    {
        egl_arosmesa_init_driver_api(&drv->base);
        drv->base.Name = "AROSMesa";
        drv->base.Unload = egl_arosmesa_unload;

        return &drv->base;
    }
    else
        return NULL;
}

static VOID CloseMesa()
{
    if (MesaBase)
    {
        CloseLibrary(MesaBase);
        MesaBase = NULL;
    }
}

ADD2EXPUNGELIB(CloseMesa, 5)
