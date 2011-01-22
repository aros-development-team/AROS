#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   /* A context is a display resource */
   _EGLResource Resource;

   /* The bound status of the context */
   _EGLThreadInfo *Binding;
   _EGLSurface *DrawSurface;
   _EGLSurface *ReadSurface;

   _EGLConfig *Config;

   EGLint ClientAPI; /**< EGL_OPENGL_ES_API, EGL_OPENGL_API, EGL_OPENVG_API */
   EGLint ClientVersion; /**< 1 = OpenGLES 1.x, 2 = OpenGLES 2.x */

   /* The real render buffer when a window surface is bound */
   EGLint WindowRenderBuffer;
};


PUBLIC EGLBoolean
_eglInitContext(_EGLContext *ctx, _EGLDisplay *dpy,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);


PUBLIC EGLBoolean
_eglBindContext(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read,
                _EGLContext **old_ctx,
                _EGLSurface **old_draw, _EGLSurface **old_read);


/**
 * Increment reference count for the context.
 */
static INLINE _EGLContext *
_eglGetContext(_EGLContext *ctx)
{
   if (ctx)
      _eglGetResource(&ctx->Resource);
   return ctx;
}


/**
 * Decrement reference count for the context.
 */
static INLINE EGLBoolean
_eglPutContext(_EGLContext *ctx)
{
   return (ctx) ? _eglPutResource(&ctx->Resource) : EGL_FALSE;
}


/**
 * Link a context to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLContext
_eglLinkContext(_EGLContext *ctx)
{
   _eglLinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
   return (EGLContext) ctx;
}


/**
 * Unlink a linked context from its display.
 * Accessing an unlinked context should generate EGL_BAD_CONTEXT error.
 */
static INLINE void
_eglUnlinkContext(_EGLContext *ctx)
{
   _eglUnlinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
}


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
static INLINE _EGLContext *
_eglLookupContext(EGLContext context, _EGLDisplay *dpy)
{
   _EGLContext *ctx = (_EGLContext *) context;
   if (!dpy || !_eglCheckResource((void *) ctx, _EGL_RESOURCE_CONTEXT, dpy))
      ctx = NULL;
   return ctx;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
static INLINE EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   _EGLResource *res = (_EGLResource *) ctx;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLContext) ctx : EGL_NO_CONTEXT;
}


#endif /* EGLCONTEXT_INCLUDED */
