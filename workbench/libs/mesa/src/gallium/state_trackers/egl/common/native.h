/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#ifndef _NATIVE_H_
#define _NATIVE_H_

#include "EGL/egl.h"  /* for EGL native types */

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/sw_winsys.h"

#include "native_modeset.h"

/**
 * Only color buffers are listed.  The others are allocated privately through,
 * for example, st_renderbuffer_alloc_storage().
 */
enum native_attachment {
   NATIVE_ATTACHMENT_FRONT_LEFT,
   NATIVE_ATTACHMENT_BACK_LEFT,
   NATIVE_ATTACHMENT_FRONT_RIGHT,
   NATIVE_ATTACHMENT_BACK_RIGHT,

   NUM_NATIVE_ATTACHMENTS
};

enum native_param_type {
   /*
    * Return TRUE if window/pixmap surfaces use the buffers of the native
    * types.
    */
   NATIVE_PARAM_USE_NATIVE_BUFFER
};

struct native_surface {
   /**
    * Available for caller's use.
    */
   void *user_data;

   void (*destroy)(struct native_surface *nsurf);

   /**
    * Swap the front and back buffers so that the back buffer is visible.  It
    * is no-op if the surface is single-buffered.  The contents of the back
    * buffer after swapping may or may not be preserved.
    */
   boolean (*swap_buffers)(struct native_surface *nsurf);

   /**
    * Make the front buffer visible.  In some native displays, changes to the
    * front buffer might not be visible immediately and require manual flush.
    */
   boolean (*flush_frontbuffer)(struct native_surface *nsurf);

   /**
    * Validate the buffers of the surface.  textures, if not NULL, points to an
    * array of size NUM_NATIVE_ATTACHMENTS and the returned textures are owned
    * by the caller.  A sequence number is also returned.  The caller can use
    * it to check if anything has changed since the last call. Any of the
    * pointers may be NULL and it indicates the caller has no interest in those
    * values.
    *
    * If this function is called multiple times with different attachment
    * masks, those not listed in the latest call might be destroyed.  This
    * behavior might change in the future.
    */
   boolean (*validate)(struct native_surface *nsurf, uint attachment_mask,
                       unsigned int *seq_num, struct pipe_resource **textures,
                       int *width, int *height);

   /**
    * Wait until all native commands affecting the surface has been executed.
    */
   void (*wait)(struct native_surface *nsurf);
};

/**
 * Describe a native display config.
 */
struct native_config {
   /* available buffers and their format */
   uint buffer_mask;
   enum pipe_format color_format;

   /* supported surface types */
   boolean window_bit;
   boolean pixmap_bit;
   boolean scanout_bit;

   int native_visual_id;
   int native_visual_type;
   int level;
   int samples;
   boolean slow_config;
   boolean transparent_rgb;
   int transparent_rgb_values[3];
};

/**
 * A pipe winsys abstracts the OS.  A pipe screen abstracts the graphcis
 * hardware.  A native display consists of a pipe winsys, a pipe screen, and
 * the native display server.
 */
struct native_display {
   /**
    * The pipe screen of the native display.
    */
   struct pipe_screen *screen;

   /**
    * Available for caller's use.
    */
   void *user_data;

   void (*destroy)(struct native_display *ndpy);

   /**
    * Query the parameters of the native display.
    *
    * The return value is defined by the parameter.
    */
   int (*get_param)(struct native_display *ndpy,
                    enum native_param_type param);

   /**
    * Get the supported configs.  The configs are owned by the display, but
    * the returned array should be FREE()ed.
    */
   const struct native_config **(*get_configs)(struct native_display *ndpy,
                                               int *num_configs);

   /**
    * Test if a pixmap is supported by the given config.  Required unless no
    * config has pixmap_bit set.
    *
    * This function is usually called to find a config that supports a given
    * pixmap.  Thus, it is usually called with the same pixmap in a row.
    */
   boolean (*is_pixmap_supported)(struct native_display *ndpy,
                                  EGLNativePixmapType pix,
                                  const struct native_config *nconf);


   /**
    * Create a window surface.  Required unless no config has window_bit set.
    */
   struct native_surface *(*create_window_surface)(struct native_display *ndpy,
                                                   EGLNativeWindowType win,
                                                   const struct native_config *nconf);

   /**
    * Create a pixmap surface.  Required unless no config has pixmap_bit set.
    */
   struct native_surface *(*create_pixmap_surface)(struct native_display *ndpy,
                                                   EGLNativePixmapType pix,
                                                   const struct native_config *nconf);

   const struct native_display_modeset *modeset;
};

/**
 * The handler for events that a native display may generate.  The events are
 * generated asynchronously and the handler may be called by any thread at any
 * time.
 */
struct native_event_handler {
   /**
    * This function is called when a surface needs to be validated.
    */
   void (*invalid_surface)(struct native_display *ndpy,
                           struct native_surface *nsurf,
                           unsigned int seq_num);

   struct pipe_screen *(*new_drm_screen)(struct native_display *ndpy,
                                         const char *name, int fd);
   struct pipe_screen *(*new_sw_screen)(struct native_display *ndpy,
                                        struct sw_winsys *ws);
};

/**
 * Test whether an attachment is set in the mask.
 */
static INLINE boolean
native_attachment_mask_test(uint mask, enum native_attachment att)
{
   return !!(mask & (1 << att));
}

struct native_platform {
   const char *name;

   struct native_display *(*create_display)(void *dpy,
                                            struct native_event_handler *handler,
                                            void *user_data);
};

const struct native_platform *
native_get_gdi_platform(void);

const struct native_platform *
native_get_x11_platform(void);

const struct native_platform *
native_get_drm_platform(void);

const struct native_platform *
native_get_fbdev_platform(void);

#endif /* _NATIVE_H_ */
