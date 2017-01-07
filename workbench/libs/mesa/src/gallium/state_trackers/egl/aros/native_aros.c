/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
 * Copyright (C) 2010-2017 The AROS Development Team. All rights reserved.
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
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

struct aros_display 
{
    struct native_display base;
    PipeHandle_t pipehandle;
    const struct native_event_handler *event_handler;
    struct native_config *configs;
    int configs_count;
};

struct aros_surface 
{
   struct native_surface base;
   struct Window * window;
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
    uint w,h;

    resource_surface_get_size(asurf->rsurf, &w, &h);

    resource_surface_get_resources(asurf->rsurf, pres, 1 << NATIVE_ATTACHMENT_BACK_LEFT);

    BltPipeResourceRastPort(asurf->adpy->pipehandle, pres[NATIVE_ATTACHMENT_BACK_LEFT], 0, 0, 
        asurf->window->RPort, asurf->window->BorderLeft, asurf->window->BorderTop,
        w, h);

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
}

static boolean
aros_surface_flush_frontbuffer(struct native_surface *nsurf)
{
    bug("[EGL] Implement flush_frontbuffer\n");
    return true;
}

static enum pipe_format
aros_display_get_format_from_window(struct Window * window)
{
    ULONG bpp = 0;

    bpp = GetCyberMapAttr(window->WScreen->RastPort.BitMap, CYBRMATTR_BPPIX);

    if (bpp == 4)
        return PIPE_FORMAT_B8G8R8A8_UNORM;
    else if (bpp == 2)
        return PIPE_FORMAT_B5G6R5_UNORM;
    else
        return PIPE_FORMAT_NONE;
}

static boolean
aros_surface_present(struct native_surface *nsurf, enum native_attachment natt,
                       boolean preserve,uint swap_interval)
{
    boolean ret;

    if (preserve || swap_interval)
        return FALSE;

    switch (natt) {
    case NATIVE_ATTACHMENT_FRONT_LEFT:
        ret = aros_surface_flush_frontbuffer(nsurf);
        break;
    case NATIVE_ATTACHMENT_BACK_LEFT:
        ret = aros_surface_swap_buffers(nsurf);
        break;
    default:
        ret = FALSE;
        break;
    }

    return ret;
}

static struct aros_surface *
aros_display_create_surface(struct native_display *ndpy,
                              struct Window * window,
                              const struct native_config *nconf)
{
    struct aros_display *adpy= aros_display(ndpy);
    struct aros_surface *asurf;

    asurf = CALLOC_STRUCT(aros_surface);
    if (!asurf)
        return NULL;

    asurf->adpy= adpy;
    asurf->color_format = aros_display_get_format_from_window(window);
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
    asurf->base.present = aros_surface_present;
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

    asurf = aros_display_create_surface(ndpy, win, nconf);
    return (asurf) ? &asurf->base : NULL;
}

static boolean
aros_display_is_format_supported(struct native_display *ndpy,
                                enum pipe_format fmt, boolean is_color)
{
    return ndpy->screen->is_format_supported(ndpy->screen,
        fmt, PIPE_TEXTURE_2D, 0,
        (is_color) ? PIPE_BIND_RENDER_TARGET :
        PIPE_BIND_DEPTH_STENCIL);
}

static const struct native_config **
aros_display_get_configs(struct native_display *ndpy, int *num_configs)
{
    struct aros_display *arosdpy = aros_display(ndpy);
    const struct native_config **configs;

    /* first time */
    if (!arosdpy->configs) 
    {
        enum pipe_format formats[2] = {PIPE_FORMAT_NONE, PIPE_FORMAT_NONE};
        int i = 0;

        arosdpy->configs_count = 0;

        /* Check two configs - 24 and 16 bit */
        if (aros_display_is_format_supported(&arosdpy->base, PIPE_FORMAT_B8G8R8A8_UNORM, true))
            formats[arosdpy->configs_count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
        if (aros_display_is_format_supported(&arosdpy->base, PIPE_FORMAT_B5G6R5_UNORM, true))
            formats[arosdpy->configs_count++] = PIPE_FORMAT_B5G6R5_UNORM;

        if (arosdpy->configs_count == 0)
            return NULL;

        arosdpy->configs = CALLOC(arosdpy->configs_count, sizeof(struct native_config));
        if (!arosdpy->configs)
            return NULL;
        
        for (i = 0; i < arosdpy->configs_count; i++)
        {
            struct native_config * nconf = &arosdpy->configs[i];
            
            nconf->buffer_mask = 
                (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
                (1 << NATIVE_ATTACHMENT_BACK_LEFT);

            nconf->color_format = formats[i];
            /* scanout makes no sense because of how AROS works full screen (new window on new screen) */
            nconf->scanout_bit = FALSE;
            nconf->pixmap_bit = FALSE;

            nconf->window_bit = TRUE;
        }

    }

    configs = MALLOC(arosdpy->configs_count * sizeof(*configs));
    if (configs) 
    {
        int i = 0;
        for (i = 0; i < arosdpy->configs_count; i++)
            configs[i] = &arosdpy->configs[i];
        if (num_configs)
            *num_configs = arosdpy->configs_count;
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

    if (arosdpy->configs)
        FREE(arosdpy->configs);

    if (arosdpy->base.screen)
        DestroyPipeScreen(arosdpy->pipehandle, arosdpy->base.screen);

    FREE(arosdpy);
}

static boolean
aros_display_init_screen(struct native_display *ndpy)
{
    struct aros_display *arosdpy = aros_display(ndpy);

    if (arosdpy->pipehandle = CreatePipeV(NULL))
    {
        arosdpy->base.screen = CreatePipeScreen(arosdpy->pipehandle);
        if (arosdpy->base.screen)
            return TRUE;
    }

    bug("%s: ERROR -  failed to create gallium pipe/screen\n", __func__);

    return FALSE;
}

static const struct native_event_handler *aros_event_handler;

static struct native_display *
native_create_display(void * dpy, boolean use_sw)
{
    struct aros_display * adpy;

    adpy = CALLOC_STRUCT(aros_display);
    if (!adpy)
        return NULL;

    adpy->event_handler = aros_event_handler;

    adpy->base.init_screen = aros_display_init_screen;
    adpy->base.destroy = aros_display_destroy;
    adpy->base.get_param = aros_display_get_param;
    adpy->base.get_configs = aros_display_get_configs;

    adpy->base.create_window_surface = aros_display_create_window_surface;
    adpy->base.create_pixmap_surface = NULL;

    return &adpy->base;
}

static const struct native_platform aros_platform = 
{
   "AROS Intuition/Graphics",
   native_create_display
};

const struct native_platform *
native_get_aros_platform(const struct native_event_handler *event_handler)
{
   aros_event_handler = event_handler;
   return &aros_platform;
}

