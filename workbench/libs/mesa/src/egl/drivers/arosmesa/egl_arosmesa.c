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

// TO REMOVE
#include "eglmisc.h"


struct egl_arosmesa
{
    _EGLDriver base;
};

struct egl_arosmesa_context
{
    _EGLContext base;
};

struct egl_arosmesa_surface
{
    _EGLSurface base;
};

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
        free(ctx);
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
    return EGL_TRUE;
}

static EGLBoolean egl_arosmesa_makecurrent(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
        _EGLSurface *rsurf, _EGLContext *ctx)
{
    _EGLContext *old_ctx;
    _EGLSurface *old_dsurf, *old_rsurf;

    if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
        return EGL_FALSE;

    return EGL_TRUE;
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

    surf->base.Width = window->Width;
    surf->base.Height = window->Height;

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
//TO UNCOMMENT
    //_eglInitDriverFallbacks(drv);

    drv->API.Initialize = egl_arosmesa_initialize;
    drv->API.CreateContext = egl_arosmesa_createcontext;
    drv->API.CreateWindowSurface = egl_arosmesa_createwindowsurface;
    drv->API.MakeCurrent = egl_arosmesa_makecurrent;
    drv->API.SwapBuffers = egl_arosmesa_swapbuffers;
    drv->API.DestroySurface = egl_arosmesa_destroysurface;
    drv->API.DestroyContext = egl_arosmesa_destroycontext;
    drv->API.Terminate = egl_arosmesa_terminate;
    // TODO: implement getprocaddress
    // drv->API.GetProcAddress

// TO REMOVE
    drv->API.QueryString = _eglQueryString;
    drv->API.ChooseConfig = _eglChooseConfig;
}

_EGLDriver *
_eglBuiltInDriverAROSMesa(const char *args)
{
    struct egl_arosmesa * drv = calloc(1, sizeof(struct egl_arosmesa));

    egl_arosmesa_init_driver_api(&drv->base);
    drv->base.Name = "AROSMesa";
    // TODO: implement unload
    drv->base.Unload = NULL;

    return &drv->base;
}
