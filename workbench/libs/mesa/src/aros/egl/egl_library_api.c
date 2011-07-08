#include <EGL/egl.h>
#include "eglapim.h"
#include "egl_intern.h"

AROS_LH0(EGLint, eglGetError,
    struct Library *, EGLBase, 35, Egl)
{
    AROS_LIBFUNC_INIT

    EGLint _return = meglGetError();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(EGLDisplay, eglGetDisplay,
    AROS_LHA(EGLNativeDisplayType, display_id, D0),
    struct Library *, EGLBase, 36, Egl)
{
    AROS_LIBFUNC_INIT

    EGLDisplay _return = meglGetDisplay(display_id);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(EGLBoolean, eglInitialize,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLint *, major, A0),
    AROS_LHA(EGLint *, minor, A1),
    struct Library *, EGLBase, 37, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglInitialize(dpy, major, minor);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(EGLBoolean, eglTerminate,
    AROS_LHA(EGLDisplay, dpy, D0),
    struct Library *, EGLBase, 38, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglTerminate(dpy);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(const char *, eglQueryString,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLint, name, D1),
    struct Library *, EGLBase, 39, Egl)
{
    AROS_LIBFUNC_INIT

    const char * _return = meglQueryString(dpy, name);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglGetConfigs,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig *, configs, A0),
    AROS_LHA(EGLint, config_size, D1),
    AROS_LHA(EGLint *, num_config, A1),
    struct Library *, EGLBase, 40, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglGetConfigs(dpy, configs, config_size, num_config);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(EGLBoolean, eglChooseConfig,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(const EGLint *, attrib_list, A0),
    AROS_LHA(EGLConfig *, configs, A1),
    AROS_LHA(EGLint, config_size, D1),
    AROS_LHA(EGLint *, num_config, A2),
    struct Library *, EGLBase, 41, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglChooseConfig(dpy, attrib_list, configs, config_size, num_config);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglGetConfigAttrib,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig, config, D1),
    AROS_LHA(EGLint, attribute, D2),
    AROS_LHA(EGLint *, value, A0),
    struct Library *, EGLBase, 42, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglGetConfigAttrib(dpy, config, attribute, value);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLSurface, eglCreateWindowSurface,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig, config, D1),
    AROS_LHA(EGLNativeWindowType, win, D2),
    AROS_LHA(const EGLint *, attrib_list, A0),
    struct Library *, EGLBase, 43, Egl)
{
    AROS_LIBFUNC_INIT

    EGLSurface _return = meglCreateWindowSurface(dpy, config, win, attrib_list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(EGLSurface, eglCreatePbufferSurface,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig, config, D1),
    AROS_LHA(const EGLint *, attrib_list, A0),
    struct Library *, EGLBase, 44, Egl)
{
    AROS_LIBFUNC_INIT

    EGLSurface _return = meglCreatePbufferSurface(dpy, config, attrib_list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLSurface, eglCreatePixmapSurface,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig, config, D1),
    AROS_LHA(EGLNativePixmapType, pixmap, D2),
    AROS_LHA(const EGLint *, attrib_list, A0),
    struct Library *, EGLBase, 45, Egl)
{
    AROS_LIBFUNC_INIT

    EGLSurface _return = meglCreatePixmapSurface(dpy, config, pixmap, attrib_list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(EGLBoolean, eglDestroySurface,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    struct Library *, EGLBase, 46, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglDestroySurface(dpy, surface);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglQuerySurface,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    AROS_LHA(EGLint, attribute, D2),
    AROS_LHA(EGLint *, value, A0),
    struct Library *, EGLBase, 47, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglQuerySurface(dpy, surface, attribute, value);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(EGLBoolean, eglBindAPI,
    AROS_LHA(EGLenum, api, D0),
    struct Library *, EGLBase, 48, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglBindAPI(api);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLenum, eglQueryAPI,
    struct Library *, EGLBase, 49, Egl)
{
    AROS_LIBFUNC_INIT

    EGLenum _return = meglQueryAPI();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLBoolean, eglWaitClient,
    struct Library *, EGLBase, 50, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglWaitClient();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLBoolean, eglReleaseThread,
    struct Library *, EGLBase, 51, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglReleaseThread();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH5(EGLSurface, eglCreatePbufferFromClientBuffer,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLenum, buftype, D1),
    AROS_LHA(EGLClientBuffer, buffer, D2),
    AROS_LHA(EGLConfig, config, D3),
    AROS_LHA(const EGLint *, attrib_list, A0),
    struct Library *, EGLBase, 52, Egl)
{
    AROS_LIBFUNC_INIT

    EGLSurface _return = meglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglSurfaceAttrib,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    AROS_LHA(EGLint, attribute, D2),
    AROS_LHA(EGLint, value, D3),
    struct Library *, EGLBase, 53, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglSurfaceAttrib(dpy, surface, attribute, value);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(EGLBoolean, eglBindTexImage,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    AROS_LHA(EGLint, buffer, D2),
    struct Library *, EGLBase, 54, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglBindTexImage(dpy, surface, buffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(EGLBoolean, eglReleaseTexImage,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    AROS_LHA(EGLint, buffer, D2),
    struct Library *, EGLBase, 55, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglReleaseTexImage(dpy, surface, buffer);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(EGLBoolean, eglSwapInterval,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLint, interval, D1),
    struct Library *, EGLBase, 56, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglSwapInterval(dpy, interval);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLContext, eglCreateContext,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLConfig, config, D1),
    AROS_LHA(EGLContext, share_context, D2),
    AROS_LHA(const EGLint *, attrib_list, A0),
    struct Library *, EGLBase, 57, Egl)
{
    AROS_LIBFUNC_INIT

    EGLContext _return = meglCreateContext(dpy, config, share_context, attrib_list);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(EGLBoolean, eglDestroyContext,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLContext, ctx, D1),
    struct Library *, EGLBase, 58, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglDestroyContext(dpy, ctx);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglMakeCurrent,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, draw, D1),
    AROS_LHA(EGLSurface, read, D2),
    AROS_LHA(EGLContext, ctx, D3),
    struct Library *, EGLBase, 59, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglMakeCurrent(dpy, draw, read, ctx);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLContext, eglGetCurrentContext,
    struct Library *, EGLBase, 60, Egl)
{
    AROS_LIBFUNC_INIT

    EGLContext _return = meglGetCurrentContext();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(EGLSurface, eglGetCurrentSurface,
    AROS_LHA(EGLint, readdraw, D0),
    struct Library *, EGLBase, 61, Egl)
{
    AROS_LIBFUNC_INIT

    EGLSurface _return = meglGetCurrentSurface(readdraw);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLDisplay, eglGetCurrentDisplay,
    struct Library *, EGLBase, 62, Egl)
{
    AROS_LIBFUNC_INIT

    EGLDisplay _return = meglGetCurrentDisplay();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(EGLBoolean, eglQueryContext,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLContext, ctx, D1),
    AROS_LHA(EGLint, attribute, D2),
    AROS_LHA(EGLint *, value, A0),
    struct Library *, EGLBase, 63, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglQueryContext(dpy, ctx, attribute, value);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(EGLBoolean, eglWaitGL,
    struct Library *, EGLBase, 64, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglWaitGL();

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(EGLBoolean, eglWaitNative,
    AROS_LHA(EGLint, engine, D0),
    struct Library *, EGLBase, 65, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglWaitNative(engine);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(EGLBoolean, eglSwapBuffers,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    struct Library *, EGLBase, 66, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglSwapBuffers(dpy, surface);

    return _return;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(EGLBoolean, eglCopyBuffers,
    AROS_LHA(EGLDisplay, dpy, D0),
    AROS_LHA(EGLSurface, surface, D1),
    AROS_LHA(EGLNativePixmapType, target, D2),
    struct Library *, EGLBase, 67, Egl)
{
    AROS_LIBFUNC_INIT

    EGLBoolean _return = meglCopyBuffers(dpy, surface, target);

    return _return;

    AROS_LIBFUNC_EXIT
}

