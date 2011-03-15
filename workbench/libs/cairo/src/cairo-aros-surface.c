/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2007 Robert Norris <rob@cataclysm.cx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Robert Norris
 * <rob@cataclysm.cx>
 *
 * Contributor(s):
 */

#include "cairoint.h"
#include "cairo-error-private.h"
#include "cairo-aros.h"

#include <exec/types.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <aros/macros.h>

#if AROS_BIG_ENDIAN
#define AROS_PIXFMT_ARGB32  PIXFMT_ARGB32
#define AROS_RECTFMT_ARGB32 RECTFMT_ARGB32
#define AROS_PIXFMT_RGB24   PIXFMT_RGB24
#define AROS_RECTFMT_RGB24  RECTFMT_RGB24
#else
#define AROS_PIXFMT_ARGB32  PIXFMT_BGRA32
#define AROS_RECTFMT_ARGB32 RECTFMT_BGRA32
#define AROS_PIXFMT_RGB24   PIXFMT_BGR24
#define AROS_RECTFMT_RGB24  RECTFMT_BGR24
#endif

typedef struct _cairo_aros_surface {
    cairo_surface_t     base;

    struct RastPort     *rastport;
    BOOL                own_rastport;

    ULONG               xoff;
    ULONG               yoff;

    ULONG               width;
    ULONG               height;

    ULONG               pixfmt;
    cairo_content_t     content;
} cairo_aros_surface_t;

static cairo_surface_t *
_cairo_aros_surface_create_similar (void            *abstract_src,
                                    cairo_content_t content,
                                    int             width,
                                    int             height)
{
    cairo_aros_surface_t *src = abstract_src, *surface;
    struct BitMap *bitmap;
    struct RastPort *rastport;
    cairo_format_t format;
    ULONG pixfmt = 0;

    format = _cairo_format_from_content (content);

    switch (format) {
        case CAIRO_FORMAT_ARGB32:
            pixfmt = AROS_PIXFMT_ARGB32;
            break;

        case CAIRO_FORMAT_RGB24:
            pixfmt = AROS_PIXFMT_RGB24;
            break;

        /* XXX implement alpha-only formats */
        case CAIRO_FORMAT_A8:
        case CAIRO_FORMAT_A1:
            return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_FORMAT));
    }

    bitmap = AllocBitMap (width, height, 0, BMF_SPECIALFMT | (pixfmt << 24), src->rastport->BitMap);
    if (bitmap == NULL) {
        return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    rastport = (struct RastPort *) malloc (sizeof (struct RastPort));
    if (rastport == NULL) {
        FreeBitMap (bitmap);
        return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    InitRastPort (rastport);
    rastport->BitMap = bitmap;

    surface = (cairo_aros_surface_t *) cairo_aros_surface_create (rastport, 0, 0, width, height);
    surface->own_rastport = TRUE;

    return &surface->base;
}

static cairo_status_t
_cairo_aros_surface_finish (void *abstract_surface)
{
    cairo_aros_surface_t *surface = abstract_surface;

    if (surface->own_rastport) {
        DeinitRastPort (surface->rastport);
        FreeBitMap (surface->rastport->BitMap);
        free (surface->rastport);
        surface->rastport = NULL;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_surface_to_image (cairo_aros_surface_t   *surface,
                   cairo_rectangle_int_t  *interest_rect,
                   cairo_image_surface_t **image_out,
                   cairo_rectangle_int_t  *image_rect)
{
    cairo_image_surface_t *image;
    int x1, y1, x2, y2;
    uint8_t *pixels;

    x1 = 0;
    y1 = 0;
    x2 = surface->width;
    y2 = surface->height;

    if (interest_rect) {
        cairo_rectangle_t rect;

        rect.x = interest_rect->x;
        rect.y = interest_rect->y;
        rect.width = interest_rect->width;
        rect.height = interest_rect->height;

        if (rect.x > x1)
            x1 = rect.x;
        if (rect.y > y1)
            y1 = rect.y;
        if (rect.x + rect.width < x2)
            x2 = rect.x + rect.width;
        if (rect.y + rect.height < y2)
            y2 = rect.y + rect.height;

        if (x1 >= x2 || y1 >= y2) {
            *image_out = NULL;
            return CAIRO_STATUS_SUCCESS;
        }
    }

    if (image_rect) {
        image_rect->x = x1;
        image_rect->y = y1;
        image_rect->width = x2 - x1;
        image_rect->height = y2 - y1;
    }

    if ((pixels = (uint8_t *) malloc ((x2-x1) * (y2-y1) * 4)) == NULL)
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    ReadPixelArray ((APTR) pixels, 0, 0, (x2-x1) * 4,
                    surface->rastport, x1 + surface->xoff, y1 + surface->yoff, x2-x1, y2-y1,
                    AROS_RECTFMT_ARGB32);
    
    /*
     * For historical reasons related to the Cybergraphics API, graphics.hidd
     * will set the alpha value to 0 when reading 24->32 bits or writing
     * 32->24 bits.  pixman will (correctly) interpret the pixels as being
     * transparent. SO before we feed the pixel data in to cairo, we have to
     * fix it by forcing the alpha value to 0xff.
     *
     * One day graphics.library will give us a way to request this from the
     * graphics.hidd format conversion routines, and this won't be necessary.
     */

    if (surface->pixfmt == PIXFMT_RGB24  || surface->pixfmt == PIXFMT_BGR24  ||
        surface->pixfmt == PIXFMT_0RGB32 || surface->pixfmt == PIXFMT_0BGR32 ||
        surface->pixfmt == PIXFMT_RGB032 || surface->pixfmt == PIXFMT_BGR032) {
        int i;
        for (i = 0; i < (x2-x1) * (y2-y1); i++)
            ((uint32_t *) pixels)[i] |= 0xff000000;

        image = (cairo_image_surface_t *)
            cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_RGB24, x2-x1, y2-y1, (x2-x1) * 4);
    }
    else
        image = (cairo_image_surface_t *)
            cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, x2-x1, y2-y1, (x2-x1) * 4);

    if (image->base.status) {
        free (pixels);
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    _cairo_image_surface_assume_ownership_of_data (image);

    *image_out = image;
    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_image_to_surface (cairo_aros_surface_t  *surface,
                   cairo_image_surface_t *image,
                   int                   src_x,
                   int                   src_y,
                   int                   width,
                   int                   height,
                   int                   dst_x,
                   int                   dst_y)
{
    WritePixelArray ((APTR) image->data, src_x, src_y, image->stride,
                    surface->rastport, dst_x + surface->xoff, dst_y + surface->yoff, width, height,
                    AROS_RECTFMT_ARGB32);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_aros_surface_acquire_source_image (void                   *abstract_surface,
                                          cairo_image_surface_t **image_out,
                                          void                  **image_extra)
{
    cairo_aros_surface_t *surface = abstract_surface;
    cairo_image_surface_t *image;
    cairo_status_t status;

    status = _surface_to_image (surface, NULL, &image, NULL);
    if (status)
        return status;

    *image_out = image;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_aros_surface_release_source_image (void                  *abstract_surface,
                                          cairo_image_surface_t *image,
                                          void                  *image_extra)
{
    cairo_surface_destroy (&image->base);
}

static cairo_status_t
_cairo_aros_surface_acquire_dest_image (void                   *abstract_surface,
                                        cairo_rectangle_int_t  *interest_rect,
                                        cairo_image_surface_t **image_out,
                                        cairo_rectangle_int_t  *image_rect_out,
                                        void                  **image_extra)
{
    cairo_aros_surface_t *surface = abstract_surface;
    cairo_image_surface_t *image;
    cairo_status_t status;

    status = _surface_to_image (surface, interest_rect, &image, image_rect_out);
    if (status)
        return status;

    *image_out = image;
    *image_extra = NULL;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_aros_surface_release_dest_image (void                  *abstract_surface,
                                        cairo_rectangle_int_t *interest_rect,
                                        cairo_image_surface_t *image,
                                        cairo_rectangle_int_t *image_rect,
                                        void                  *image_extra)
{

    cairo_aros_surface_t *surface = abstract_surface;
    cairo_status_t status;

    status = _image_to_surface (surface, image, 0, 0, image->width, image->height, image_rect->x, image_rect->y);
    _cairo_surface_set_error (&surface->base, status);

    cairo_surface_destroy (&image->base);
}

static cairo_int_status_t
_cairo_aros_surface_get_extents (void                  *abstract_surface,
                                 cairo_rectangle_int_t *rectangle)
{
    cairo_aros_surface_t *surface = abstract_surface;

    rectangle->x = 0;
    rectangle->y = 0;

    rectangle->width =  surface->width;
    rectangle->height = surface->height;

    return CAIRO_STATUS_SUCCESS;
}

static const cairo_surface_backend_t cairo_aros_surface_backend = {
    CAIRO_SURFACE_TYPE_AROS,
    _cairo_aros_surface_create_similar,
    _cairo_aros_surface_finish,
    _cairo_aros_surface_acquire_source_image,
    _cairo_aros_surface_release_source_image,
    _cairo_aros_surface_acquire_dest_image,
    _cairo_aros_surface_release_dest_image,
    NULL, /* clone_similar */
    NULL, /* composite */
    NULL, /* fill_rectangles */
    NULL, /* composite_trapezoids */
    NULL, /* copy_page */
    NULL, /* show_page */
    NULL, /* set_clip_region */
    NULL, /* intersect_clip_path */
    _cairo_aros_surface_get_extents,
    NULL, /* old_show_glyphs */
    NULL, /* get_font_options */
    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */
    NULL, /* scaled_font_fini */
    NULL, /* scaled_glyph_fini */
    NULL, /* paint */
    NULL, /* mask */
    NULL, /* stroke */
    NULL, /* fill */
    NULL, /* show_glyphs */
    NULL, /* snapshot */
    NULL, /* is_similar */
    NULL, /* reset */
    NULL, /* fill_stroke */
};

cairo_surface_t *
cairo_aros_surface_create (struct RastPort *rastport, int xoff, int yoff, int width, int height)
{
    cairo_aros_surface_t *surface;

    surface = (cairo_aros_surface_t *) malloc (sizeof (cairo_aros_surface_t));

    surface->rastport = rastport;
    surface->own_rastport = FALSE;

    surface->xoff = xoff;
    surface->yoff = yoff;

    surface->width = width;
    surface->height = height;

    surface->pixfmt = GetCyberMapAttr (rastport->BitMap, CYBRMATTR_PIXFMT_ALPHA);

    if (surface->pixfmt == PIXFMT_ARGB32 ||
        surface->pixfmt == PIXFMT_BGRA32 || 
        surface->pixfmt == PIXFMT_RGBA32 || 
        surface->pixfmt == PIXFMT_ABGR32)
    {
        surface->content = CAIRO_CONTENT_COLOR_ALPHA;
    }
    else
    {
        surface->content = CAIRO_CONTENT_COLOR;
    }

    _cairo_surface_init (&surface->base, &cairo_aros_surface_backend, NULL, surface->content);

    return &surface->base;
}

void cairo_aros_surface_set_extents (cairo_surface_t *surface, int xoff, int yoff, int width, int height) {
    cairo_aros_surface_t *aros_surface = (cairo_aros_surface_t *) surface;

    aros_surface->xoff = xoff;
    aros_surface->yoff = yoff;

    aros_surface->width = width;
    aros_surface->height = height;
}
