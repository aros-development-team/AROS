/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "native.h"
#include "egl_g3d.h"
#include "egl_g3d_api.h"
#include "egl_g3d_image.h"

/* move this to native display? */
#include "state_tracker/drm_driver.h"

/**
 * Reference and return the front left buffer of the native pixmap.
 */
static struct pipe_resource *
egl_g3d_reference_native_pixmap(_EGLDisplay *dpy, EGLNativePixmapType pix)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf;
   struct native_surface *nsurf;
   struct pipe_resource *textures[NUM_NATIVE_ATTACHMENTS];
   enum native_attachment natt;

   gconf = egl_g3d_config(egl_g3d_find_pixmap_config(dpy, pix));
   if (!gconf)
      return NULL;

   nsurf = gdpy->native->create_pixmap_surface(gdpy->native,
         pix, gconf->native);
   if (!nsurf)
      return NULL;

   natt = NATIVE_ATTACHMENT_FRONT_LEFT;
   if (!nsurf->validate(nsurf, 1 << natt, NULL, textures, NULL, NULL))
      textures[natt] = NULL;

   nsurf->destroy(nsurf);

   return textures[natt];
}

#ifdef EGL_MESA_drm_image

static struct pipe_resource *
egl_g3d_create_drm_buffer(_EGLDisplay *dpy, const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_resource templ;
   EGLint width = 0, height = 0, format = 0, use = 0;
   EGLint valid_use;
   EGLint i, err = EGL_SUCCESS;

   for (i = 0; attribs[i] != EGL_NONE; i++) {
      EGLint attr = attribs[i++];
      EGLint val = attribs[i];

      switch (attr) {
      case EGL_WIDTH:
	 width = val;
         break;
      case EGL_HEIGHT:
	 height = val;
         break;
      case EGL_DRM_BUFFER_FORMAT_MESA:
	 format = val;
         break;
      case EGL_DRM_BUFFER_USE_MESA:
	 use = val;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad image attribute 0x%04x", attr);
         return NULL;
      }
   }

   if (width <= 0 || height <= 0) {
      _eglLog(_EGL_DEBUG, "bad width or height (%dx%d)", width, height);
      return NULL;
   }

   switch (format) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      _eglLog(_EGL_DEBUG, "bad image format value 0x%04x", format);
      return NULL;
      break;
   }

   valid_use = EGL_DRM_BUFFER_USE_SCANOUT_MESA |
               EGL_DRM_BUFFER_USE_SHARE_MESA;
   if (use & ~valid_use) {
      _eglLog(_EGL_DEBUG, "bad image use bit 0x%04x", use);
      return NULL;
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;

   /*
    * XXX fix apps (e.g. wayland) and pipe drivers (e.g. i915) and remove the
    * size check
    */
   if ((use & EGL_DRM_BUFFER_USE_SCANOUT_MESA) &&
       width >= 640 && height >= 480)
      templ.bind |= PIPE_BIND_SCANOUT;
   if (use & EGL_DRM_BUFFER_USE_SHARE_MESA)
      templ.bind |= PIPE_BIND_SHARED;

   return screen->resource_create(screen, &templ);
}

static struct pipe_resource *
egl_g3d_reference_drm_buffer(_EGLDisplay *dpy, EGLint name,
                             const EGLint *attribs)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct pipe_screen *screen = gdpy->native->screen;
   struct pipe_resource templ;
   struct winsys_handle wsh;
   EGLint width = 0, height = 0, format = 0, stride = 0;
   EGLint i, err = EGL_SUCCESS;

   /* winsys_handle is in theory platform-specific */
   if (dpy->Platform != _EGL_PLATFORM_DRM)
      return NULL;

   for (i = 0; attribs[i] != EGL_NONE; i++) {
      EGLint attr = attribs[i++];
      EGLint val = attribs[i];

      switch (attr) {
      case EGL_WIDTH:
	 width = val;
         break;
      case EGL_HEIGHT:
	 height = val;
         break;
      case EGL_DRM_BUFFER_FORMAT_MESA:
	 format = val;
         break;
      case EGL_DRM_BUFFER_STRIDE_MESA:
	 stride = val;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad image attribute 0x%04x", attr);
         return NULL;
      }
   }

   if (width <= 0 || height <= 0 || stride <= 0) {
      _eglLog(_EGL_DEBUG, "bad width, height, or stride (%dx%dx%d)",
            width, height, stride);
      return NULL;
   }

   switch (format) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   default:
      _eglLog(_EGL_DEBUG, "bad image format value 0x%04x", format);
      return NULL;
      break;
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;

   memset(&wsh, 0, sizeof(wsh));
   wsh.handle = (unsigned) name;
   wsh.stride = stride;

   return screen->resource_from_handle(screen, &templ, &wsh);
}

#endif /* EGL_MESA_drm_image */

_EGLImage *
egl_g3d_create_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                     EGLenum target, EGLClientBuffer buffer,
                     const EGLint *attribs)
{
   struct pipe_resource *ptex;
   struct egl_g3d_image *gimg;
   unsigned face = 0, level = 0, zslice = 0;

   gimg = CALLOC_STRUCT(egl_g3d_image);
   if (!gimg) {
      _eglError(EGL_BAD_ALLOC, "eglCreateEGLImageKHR");
      return NULL;
   }

   if (!_eglInitImage(&gimg->base, dpy, attribs)) {
      FREE(gimg);
      return NULL;
   }

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      ptex = egl_g3d_reference_native_pixmap(dpy,
            (EGLNativePixmapType) buffer);
      break;
#ifdef EGL_MESA_drm_image
   case EGL_DRM_BUFFER_MESA:
      ptex = egl_g3d_reference_drm_buffer(dpy, (EGLint) buffer, attribs);
      break;
#endif
   default:
      ptex = NULL;
      break;
   }

   if (!ptex) {
      FREE(gimg);
      return NULL;
   }

   if (level > ptex->last_level) {
      _eglError(EGL_BAD_MATCH, "eglCreateEGLImageKHR");
      pipe_resource_reference(&gimg->texture, NULL);
      FREE(gimg);
      return NULL;
   }
   if (zslice > ptex->depth0) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      pipe_resource_reference(&gimg->texture, NULL);
      FREE(gimg);
      return NULL;
   }

   /* transfer the ownership to the image */
   gimg->texture = ptex;
   gimg->face = face;
   gimg->level = level;
   gimg->zslice = zslice;

   return &gimg->base;
}

EGLBoolean
egl_g3d_destroy_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img)
{
   struct egl_g3d_image *gimg = egl_g3d_image(img);

   pipe_resource_reference(&gimg->texture, NULL);
   FREE(gimg);

   return EGL_TRUE;
}

_EGLImage *
egl_g3d_create_drm_image(_EGLDriver *drv, _EGLDisplay *dpy,
                         const EGLint *attribs)
{
   struct egl_g3d_image *gimg;
   struct pipe_resource *ptex;

   gimg = CALLOC_STRUCT(egl_g3d_image);
   if (!gimg) {
      _eglError(EGL_BAD_ALLOC, "eglCreateDRMImageKHR");
      return NULL;
   }

   if (!_eglInitImage(&gimg->base, dpy, attribs)) {
      FREE(gimg);
      return NULL;
   }

#ifdef EGL_MESA_drm_image
   ptex = egl_g3d_create_drm_buffer(dpy, attribs);
#else
   ptex = NULL;
#endif
   if (!ptex) {
      FREE(gimg);
      return NULL;
   }

   /* transfer the ownership to the image */
   gimg->texture = ptex;
   gimg->face = 0;
   gimg->level = 0;
   gimg->zslice = 0;

   return &gimg->base;
}

EGLBoolean
egl_g3d_export_drm_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img,
			 EGLint *name, EGLint *handle, EGLint *stride)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_image *gimg = egl_g3d_image(img);
   struct pipe_screen *screen = gdpy->native->screen;
   struct winsys_handle wsh;

   /* winsys_handle is in theory platform-specific */
   if (dpy->Platform != _EGL_PLATFORM_DRM)
      return EGL_FALSE;

   /* get shared handle */
   if (name) {
      memset(&handle, 0, sizeof(handle));
      wsh.type = DRM_API_HANDLE_TYPE_SHARED;
      if (!screen->resource_get_handle(screen, gimg->texture, &wsh)) {
         return EGL_FALSE;
      }

      *name = wsh.handle;
   }

   /* get KMS handle */
   if (handle || stride) {
      memset(&wsh, 0, sizeof(wsh));
      wsh.type = DRM_API_HANDLE_TYPE_KMS;
      if (!screen->resource_get_handle(screen, gimg->texture, &wsh))
         return EGL_FALSE;

      if (handle)
         *handle = wsh.handle;
      if (stride)
         *stride = wsh.stride;
   }

   return EGL_TRUE;
}
