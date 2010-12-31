/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
 * Copyright (C) 2010 The AROS Development Team. All rights reserved.
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

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "egllog.h"

#include "common/native.h"
#include "common/native_helper.h"

#include <gallium/gallium.h>
#include <proto/gallium.h>
#include <aros/debug.h>

enum aros_surface_type 
{
   AROS_SURFACE_TYPE_WINDOW,
   AROS_SURFACE_TYPE_BITMAP,
};

struct aros_config;

struct aros_display 
{
    struct native_display base;
    struct native_event_handler *event_handler;
    struct aros_config *config;
};

struct aros_config 
{
    struct native_config base;
};

struct aros_surface 
{
   struct native_surface base;
   struct Window * window;
   enum aros_surface_type type;
   enum pipe_format color_format;
   struct aros_display *adpy;

   struct resource_surface *rsurf;
};

static INLINE struct aros_display *
aros_display(const struct native_display *ndpy)
{
    return (struct aros_display *) ndpy;
}

static INLINE struct aros_config *
aros_config(const struct native_config *nconf)
{
    return (struct aros_config *) nconf;
}

static INLINE struct aros_surface *
aros_surface(const struct native_surface *nsurf)
{
    return (struct aros_surface *) nsurf;
}

static void
aros_surface_update_geometry(struct native_surface *nsurf)
{
    struct aros_surface *asurf = aros_surface(nsurf);

    resource_surface_set_size(asurf->rsurf, 
        asurf->window->RPort->Layer->bounds.MaxX - asurf->window->RPort->Layer->bounds.MinX + 1 
        - asurf->window->BorderLeft - asurf->window->BorderRight,
        asurf->window->RPort->Layer->bounds.MaxY - asurf->window->RPort->Layer->bounds.MinY + 1
        - asurf->window->BorderTop - asurf->window->BorderBottom);
}

static void
aros_surface_destroy(struct native_surface *nsurf)
{
    struct aros_surface *asurf = aros_surface(nsurf);

    resource_surface_destroy(asurf->rsurf);
    FREE(asurf);
}

static boolean
aros_surface_swap_buffers(struct native_surface *nsurf)
{
    struct aros_surface *asurf = aros_surface(nsurf);
    struct pipe_resource * pres[NUM_NATIVE_ATTACHMENTS] = {0};
    struct pipe_surface * psurf = NULL;
    uint w,h;

    resource_surface_get_size(asurf->rsurf, &w, &h);

    resource_surface_get_resources(asurf->rsurf, pres, 1 << NATIVE_ATTACHMENT_BACK_LEFT);

    psurf = asurf->adpy->base.screen->get_tex_surface(asurf->adpy->base.screen, 
                pres[NATIVE_ATTACHMENT_BACK_LEFT], 0, 0, 0, PIPE_BIND_RENDER_TARGET);

    BltPipeSurfaceRastPort(psurf, 0, 0, 
        asurf->window->RPort, asurf->window->BorderLeft, asurf->window->BorderTop,
        w, h);

    pipe_surface_reference(&psurf, NULL);
    pipe_resource_reference(&pres[NATIVE_ATTACHMENT_BACK_LEFT], NULL);
    /* TODO : call -> possible size change */
    //aros_surface_invalidate(&asurf->base);

    return true;
}

static boolean
aros_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
    struct aros_surface *asurf = aros_surface(nsurf);

//    if (xsurf->client_stamp != xsurf->server_stamp) {
    aros_surface_update_geometry(&asurf->base);
//    xsurf->client_stamp = xsurf->server_stamp;
//    }

    return resource_surface_add_resources(asurf->rsurf, buffer_mask);
}

static boolean
aros_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                        unsigned int *seq_num, struct pipe_resource **textures,
                        int *width, int *height)
{
    struct aros_surface *asurf = aros_surface(nsurf);
    uint w, h;

    if (!aros_surface_update_buffers(&asurf->base, attachment_mask))
        return FALSE;

    /*   if (seq_num)
    *seq_num = xsurf->client_stamp;*/

    if (textures)
        resource_surface_get_resources(asurf->rsurf, textures, attachment_mask);

    resource_surface_get_size(asurf->rsurf, &w, &h);
    if (width)
        *width = w;
    if (height)
        *height = h;

    return TRUE;
}

static void
aros_surface_wait(struct native_surface *nsurf)
{
//   struct ximage_surface *xsurf = ximage_surface(nsurf);
//   XSync(xsurf->xdpy->dpy, FALSE);
   /* TODO XGetImage and update the front texture */
}

static boolean
aros_surface_flush_frontbuffer(struct native_surface *nsurf)
{
//   struct ximage_surface *xsurf = ximage_surface(nsurf);
//   boolean ret;

//   ret = resource_surface_present(xsurf->rsurf,
//         NATIVE_ATTACHMENT_FRONT_LEFT, (void *) &xsurf->xdraw);
   /* force buffers to be updated in next validation call */
//   ximage_surface_invalidate(&xsurf->base);

//   return ret;
bug("aros_surface_flush_frontbuffer\n");
    return true;
}

static struct aros_surface *
aros_display_create_surface(struct native_display *ndpy,
                              enum aros_surface_type type,
                              struct Window * window,
                              const struct native_config *nconf)
{
    struct aros_display *adpy= aros_display(ndpy);
    struct aros_config *aconf = aros_config(nconf);
    struct aros_surface *asurf;

    asurf = CALLOC_STRUCT(aros_surface);
    if (!asurf)
        return NULL;

    asurf->adpy= adpy;
    asurf->type = type;
    asurf->color_format = aconf->base.color_format;
    asurf->window = window;

    asurf->rsurf = resource_surface_create(adpy->base.screen,
                        asurf->color_format,
                        PIPE_BIND_RENDER_TARGET |
                        PIPE_BIND_SAMPLER_VIEW |
                        PIPE_BIND_DISPLAY_TARGET |
                        PIPE_BIND_SCANOUT);

    if (!asurf->rsurf) 
    {
        FREE(asurf);
        return NULL;
    }

    /* initialize the geometry */
    aros_surface_update_geometry(&asurf->base);

    asurf->base.destroy = aros_surface_destroy;
    asurf->base.swap_buffers = aros_surface_swap_buffers;
    asurf->base.flush_frontbuffer = aros_surface_flush_frontbuffer;
    asurf->base.validate = aros_surface_validate;
    asurf->base.wait = aros_surface_wait;

    return asurf;
}

static struct native_surface *
aros_display_create_window_surface(struct native_display *ndpy,
                                     EGLNativeWindowType win,
                                     const struct native_config *nconf)
{
    struct aros_surface *asurf;

    asurf = aros_display_create_surface(ndpy, AROS_SURFACE_TYPE_WINDOW, win, nconf);
    return (asurf) ? &asurf->base : NULL;
}

static struct native_surface *
aros_display_create_pixmap_surface(struct native_display *ndpy,
                                     EGLNativePixmapType pix,
                                     const struct native_config *nconf)
{
/*   struct aros_surface *asurf;

   asurf = aros_display_create_surface(ndpy, AROS_SURFACE_TYPE_BITMAP,
         (struc) pix, nconf);
   return (asurf) ? &asurf->base : NULL;*/
   return NULL;
}







static boolean
aros_display_is_format_supported(struct native_display *ndpy,
                                enum pipe_format fmt, boolean is_color)
{
    return ndpy->screen->is_format_supported(ndpy->screen,
        fmt, PIPE_TEXTURE_2D, 0,
        (is_color) ? PIPE_BIND_RENDER_TARGET :
        PIPE_BIND_DEPTH_STENCIL, 0);
}

static const struct native_config **
aros_display_get_configs(struct native_display *ndpy, int *num_configs)
{
    struct aros_display *arosdpy = aros_display(ndpy);
    const struct native_config **configs;

    /* first time */
    if (!arosdpy->config) 
    {
        struct native_config *nconf;
        enum pipe_format format;

        arosdpy->config = CALLOC(1, sizeof(*arosdpy->config));
        if (!arosdpy->config)
            return NULL;

        nconf = &arosdpy->config->base;

        nconf->buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);

        format = PIPE_FORMAT_B8G8R8A8_UNORM;
        if (!aros_display_is_format_supported(&arosdpy->base, format, TRUE)) 
        {
            format = PIPE_FORMAT_A8R8G8B8_UNORM;
            if (!aros_display_is_format_supported(&arosdpy->base, format, TRUE))
                format = PIPE_FORMAT_NONE;
        }
        if (format == PIPE_FORMAT_NONE) 
        {
            FREE(arosdpy->config);
            arosdpy->config = NULL;
            return NULL;
        }

        nconf->color_format = format;

        /* FIXME: hardcoded, the driver should be able to handle window and pixmap */
        /* scanout makes no sense because of how AROS works full screen (new window on new screen) */
        nconf->scanout_bit = FALSE;
        nconf->window_bit = TRUE;
        nconf->pixmap_bit = FALSE;
    }

    configs = MALLOC(sizeof(*configs));
    if (configs) 
    {
        configs[0] = &arosdpy->config->base;
        if (num_configs)
            *num_configs = 1;
    }

    return configs;
}

static int
aros_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
    int val;

    switch (param) 
    {
    default:
        val = 0;
        break;
    }

    return val;
}

static void
aros_display_destroy(struct native_display *ndpy)
{
    struct aros_display *arosdpy = aros_display(ndpy);

    if (arosdpy->config)
        FREE(arosdpy->config);

    if (arosdpy->base.screen)
        arosdpy->base.screen->destroy(arosdpy->base.screen);

    FREE(arosdpy);
}

static struct native_display *
native_create_display(void * dpy, struct native_event_handler * event_handler,
                      void * user_data)
{
    struct aros_display * adpy;

    adpy = CALLOC_STRUCT(aros_display);
    if (!adpy)
        return NULL;

    adpy->event_handler = event_handler;
    adpy->base.user_data = user_data;

    if (!(adpy->base.screen = CreatePipeScreenV(NULL))) 
    {
        aros_display_destroy(&adpy->base);
        return NULL;
    }

    adpy->base.destroy = aros_display_destroy;
    adpy->base.get_param = aros_display_get_param;
    adpy->base.get_configs = aros_display_get_configs;
    adpy->base.create_window_surface = aros_display_create_window_surface;
    adpy->base.create_pixmap_surface = aros_display_create_pixmap_surface;

    return &adpy->base;
}

static const struct native_platform aros_platform = 
{
   "AROS Intuition/Graphics",
   native_create_display
};

const struct native_platform *
native_get_aros_platform(void)
{
   return &aros_platform;
}

