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
#include "cairo-aros.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <diskfont/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>
#include <utility/tagitem.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/bullet.h>

#include <stdio.h>
#include <string.h>

typedef struct _cairo_aros_scaled_font {
    cairo_scaled_font_t     base;

    struct GlyphEngine      *engine;
    BOOL                    own_engine;

    BOOL                    antialias;

    double                  xscale, yscale;
    double                  xspace;
} cairo_aros_scaled_font_t;

typedef struct _cairo_aros_font_face {
    cairo_font_face_t       base;

    struct TagItem          *otag;
    BOOL                    own_otag;

    BOOL                    needs_italic;
    BOOL                    needs_bold;
} cairo_aros_font_face_t;


#define FIXED1          ((FIXED) 0x00010000L)
#define FIXEDTOFLOAT(a) ((float) (a) / FIXED1)
#define FLOATTOFIXED(a) ((FIXED) ((float) (a) * FIXED1))


static struct GlyphEngine *
_get_engine_for_otag (struct TagItem *otag)
{
    char *ot_engine;
    char filename[1024];
    struct Library *BulletBase;
    struct GlyphEngine *engine;

    ot_engine = (char *) GetTagData (OT_Engine, (IPTR) NULL, otag);
    if (ot_engine == NULL)
        return NULL;

    snprintf (filename, sizeof (filename), "%s.library", ot_engine);

    BulletBase = OpenLibrary ((STRPTR) filename, 0);
    if (BulletBase == NULL)
        return NULL;

    engine = OpenEngine ();
    if (engine == NULL) {
        CloseLibrary (BulletBase);
        return NULL;
    }

    if (SetInfo (engine, OT_OTagList, otag, TAG_DONE) != 0) {
        CloseEngine (engine);
        CloseLibrary (BulletBase);
        return NULL;
    }

    return engine;
}

static void
_release_engine (struct GlyphEngine *engine) {
    struct Library *BulletBase;

    BulletBase = engine->gle_Library;
    CloseEngine (engine);
    CloseLibrary (BulletBase);
}

static void
_set_font_size (cairo_aros_scaled_font_t *font,
                FIXED height)
{
    struct Library *BulletBase = font->engine->gle_Library;

    SetInfo (font->engine, OT_PointHeight, height, TAG_DONE);
}

static void
_setup_face_options (cairo_aros_scaled_font_t *font,
                     cairo_aros_font_face_t *face)
{
    struct Library *BulletBase;

    BulletBase = font->engine->gle_Library;

    if (face->needs_italic) {
        SetInfo (font->engine, OT_ShearSin, 0x4690,
                               OT_ShearCos, 0xf615,
                               TAG_DONE);
    }

    if (face->needs_bold) {
        SetInfo (font->engine, OT_EmboldenX, 0xe75,
                               OT_EmboldenY, 0x99e,
                               TAG_DONE);
    }

    font->xspace = GetTagData (OT_SpaceWidth, 0, face->otag) / 65536.0 * font->xscale;
}

static struct GlyphMap *
_get_glyph_map (cairo_aros_scaled_font_t *font,
                uint16_t                  code)
{
    struct Library *BulletBase = font->engine->gle_Library;
    struct GlyphMap *gm;

    SetInfo (font->engine, OT_GlyphCode, code, TAG_DONE);

    if (font->antialias)
        ObtainInfo (font->engine, OT_GlyphMap8Bits, &gm, TAG_DONE);
    else
        ObtainInfo (font->engine, OT_GlyphMap, &gm, TAG_DONE);

    return gm;
}

static void
_release_glyph_map (cairo_aros_scaled_font_t *font,
                    struct GlyphMap          *gm)
{
    struct Library *BulletBase = font->engine->gle_Library;

    if (gm != NULL)
        ReleaseInfo (font->engine, OT_GlyphMap, gm, TAG_DONE);
}

static void
_cairo_aros_font_face_destroy (void *abstract_face)
{
    cairo_aros_font_face_t *face = abstract_face;

    if (face == NULL)
        return;

    if (face->own_otag)
        free (face->otag);

    face->otag = NULL;
    face->own_otag = FALSE;
}

static cairo_status_t
_cairo_aros_font_face_scaled_font_create (void                        *abstract_face,
                                          const cairo_matrix_t        *font_matrix,
                                          const cairo_matrix_t        *ctm,
                                          const cairo_font_options_t  *font_options,
                                          cairo_scaled_font_t        **font_out)
{
    cairo_aros_font_face_t *face = abstract_face;
    cairo_status_t status;
    struct GlyphEngine *engine;
    cairo_matrix_t scale;
    double xscale, yscale;
    cairo_aros_scaled_font_t *font;
    struct GlyphMap *gm;
    cairo_font_extents_t extents;

    cairo_matrix_multiply (&scale, font_matrix, ctm);
    status = _cairo_matrix_compute_scale_factors (&scale, &xscale, &yscale, 1);
    if (status)
        return status;

    engine = _get_engine_for_otag (face->otag);
    if (engine == NULL)
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font = malloc (sizeof (cairo_aros_scaled_font_t));
    if (font == NULL)
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font->engine = engine;
    font->own_engine = TRUE;

    font->xscale = xscale;
    font->yscale = yscale;

    _set_font_size (font, FLOATTOFIXED(font->yscale));

    /* XXX handle font width, rotation, whatever else */

    _setup_face_options (font, face);

    switch (font_options->antialias) {
        case CAIRO_ANTIALIAS_DEFAULT:
        case CAIRO_ANTIALIAS_NONE:
            font->antialias = FALSE;
            break;

        case CAIRO_ANTIALIAS_GRAY:
        case CAIRO_ANTIALIAS_SUBPIXEL:
            font->antialias = TRUE;
            break;
    }

    status = _cairo_scaled_font_init (&font->base, &face->base,
                                      font_matrix, ctm, font_options,
                                      &cairo_aros_scaled_font_backend);

    if (status) {
        _release_engine (font->engine);
        free (font);
        return status;
    }

    /* XXX the bullet interface does not give us any way to get the font
     * extents. instead we must derive them from a few well-chosen glyphs and
     * a little bit of fudging. this is really really bad. */
    gm = _get_glyph_map (font, (uint16_t) 'A');
    extents.ascent = FIXEDTOFLOAT(gm->glm_YOrigin) / font->yscale;
    _release_glyph_map (font, gm);

    gm = _get_glyph_map (font, (uint16_t) 'j');
    extents.descent = (gm->glm_BlackHeight - FIXEDTOFLOAT(gm->glm_YOrigin)) / font->yscale;
    _release_glyph_map (font, gm);

    extents.height = extents.ascent * 1.2 + extents.descent;

    gm = _get_glyph_map (font, (uint16_t) 'w');
    extents.max_x_advance = FIXEDTOFLOAT(gm->glm_Width);
    _release_glyph_map (font, gm);

    extents.max_y_advance = 0;

    _cairo_scaled_font_set_metrics (&font->base, &extents);

    *font_out = &font->base;
    return CAIRO_STATUS_SUCCESS;
}

static const cairo_font_face_backend_t _cairo_aros_font_face_backend = {
    CAIRO_FONT_TYPE_AROS,
    _cairo_aros_font_face_destroy,
    _cairo_aros_font_face_scaled_font_create
};

cairo_font_face_t *
cairo_aros_font_face_create_for_outline_tags (struct TagItem *tags)
{
    cairo_aros_font_face_t *face;

    face = malloc (sizeof (cairo_aros_font_face_t));
    if (face == NULL) {
        _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
        return (cairo_font_face_t *) &_cairo_font_face_nil;
    }

    face->otag = tags;
    face->own_otag = FALSE;
    face->needs_italic = face->needs_bold = FALSE;

    _cairo_font_face_init (&face->base, &_cairo_aros_font_face_backend);

    return &face->base;
}

static BOOL
_validate_otag (struct TagItem *otag, int size)
{
    struct TagItem *tag;

    if (AROS_LONG2BE (otag[0].ti_Tag) != OT_FileIdent ||
        AROS_LONG2BE (otag[0].ti_Data) != size)
        return FALSE;

    for (tag = otag; AROS_LONG2BE (tag->ti_Tag) != TAG_DONE; tag ++) {
        tag->ti_Tag = AROS_LONG2BE (tag->ti_Tag);
        tag->ti_Data = AROS_LONG2BE (tag->ti_Data);

        if (tag->ti_Tag & OT_Indirect)
            tag->ti_Data += (IPTR) otag;
    }

    return TRUE;
}

cairo_font_face_t *
cairo_aros_font_face_create (const char          *wanted_family,
                             cairo_font_slant_t   wanted_slant,
                             cairo_font_weight_t  wanted_weight)
{
    BPTR fontdir;
    struct FileInfoBlock *fib;
    cairo_aros_font_face_t *face = NULL, best, new;

    best.otag = NULL;

    /* XXX Fonts: may be a multidirectory assign, so this needs to be a
     * GetDeviceProc() loop */
    if ((fontdir = Lock ((STRPTR) "Fonts:", SHARED_LOCK)) == NULL)
        goto _face_create_finish;

    fib = AllocDosObject (DOS_FIB, NULL);
    if (fib == NULL)
        goto _face_create_unlock;

    if (!Examine (fontdir, fib))
        goto _face_create_free;

    if (fib->fib_DirEntryType < 0)
        goto _face_create_free;

    while (ExNext (fontdir, fib)) {
        char filename[1024], *ot_family;
        int namelen, size;
        struct TagItem *otag;
        BPTR fh;
        
        namelen = strlen ((char *) fib->fib_FileName);
        if (namelen < 5 || strcmp ((char *) &(fib->fib_FileName[namelen-5]), ".otag"))
            continue;

        size = fib->fib_Size;

        snprintf (filename, sizeof (filename), "Fonts:%s", fib->fib_FileName);
        if ((fh = Open ((STRPTR) filename, MODE_OLDFILE)) == NULL)
            continue;

        otag = malloc (size);
        if (otag == NULL) {
            Close (fh);
            continue;
        }

        if (Read (fh, otag, size) != size) {
            free (otag);
            Close (fh);
            continue;
        }

        Close (fh);

        if (!_validate_otag (otag, size)) {
            free (otag);
            continue;
        }

        ot_family = (char *) GetTagData (OT_Family, (IPTR) NULL, otag);
        if (ot_family != NULL && strcmp (ot_family, wanted_family) == 0) {
            int ot_slant, ot_weight;
            cairo_font_slant_t cr_slant;
            cairo_font_weight_t cr_weight;

            ot_slant = (int) GetTagData (OT_SlantStyle, (IPTR) 0, otag);
            switch (ot_slant) {
                case OTS_Upright:
                default:
                    cr_slant = CAIRO_FONT_SLANT_NORMAL;
                    break;

                case OTS_Italic:
                    cr_slant = CAIRO_FONT_SLANT_ITALIC;
                    break;

                case OTS_LeftItalic:
                    cr_slant = CAIRO_FONT_SLANT_OBLIQUE;
                    break;
            }

            ot_weight = (int) GetTagData (OT_StemWeight, (IPTR) 0, otag);
            if (ot_weight >= OTS_Bold)
                cr_weight = CAIRO_FONT_WEIGHT_BOLD;
            else
                cr_weight = CAIRO_FONT_WEIGHT_NORMAL;

            if (cr_slant == wanted_slant && cr_weight == wanted_weight) {
                if (best.otag != NULL)
                    free (best.otag);

                best.otag = otag;
                best.needs_italic = best.needs_bold = FALSE;

                goto _face_create_finish;
            }

            new.otag = otag;

            new.needs_italic = new.needs_bold = FALSE;

            if (cr_weight == CAIRO_FONT_WEIGHT_NORMAL) {
                if (wanted_weight == CAIRO_FONT_WEIGHT_BOLD)
                    new.needs_bold = TRUE;
            }
            else
                if (wanted_weight != CAIRO_FONT_WEIGHT_BOLD) {
                    free (otag);
                    continue;
                }

            if (cr_slant == CAIRO_FONT_SLANT_NORMAL) {
                if (wanted_slant > CAIRO_FONT_SLANT_NORMAL)
                    new.needs_italic = TRUE;
            }
            else
                if (wanted_slant == CAIRO_FONT_SLANT_NORMAL) {
                    free (otag);
                    continue;
                }

            if (best.otag != NULL) {

                /* if the existing best is the same as the new best:
                 *   - needs bold and italic
                 *   - needs bold only
                 *   - needs italic only
                 * then the new is not substantially better, so drop it
                 */
                if ((best.needs_italic && best.needs_bold &&
                     !(new.needs_italic ^ new.needs_bold)) ||
                    (best.needs_italic && new.needs_italic) ||
                    (best.needs_bold && new.needs_bold))
                {
                    free (otag);
                    continue;
                }

                free (best.otag);
                best = new;
            }

            else
                best = new;
        }

        else
            free (otag);
    }

_face_create_free:
    FreeDosObject (DOS_FIB, fib);

_face_create_unlock:
    UnLock (fontdir);

_face_create_finish:
    if (best.otag == NULL) {
        _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
        return (cairo_font_face_t *) &_cairo_font_face_nil;
    }

    face = malloc (sizeof (cairo_aros_font_face_t));
    if (face == NULL) {
        _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
        return (cairo_font_face_t *) &_cairo_font_face_nil;
    }

    face->otag = best.otag;
    face->own_otag = TRUE;

    face->needs_italic = best.needs_italic;
    face->needs_bold = best.needs_bold;

    _cairo_font_face_init (&face->base, &_cairo_aros_font_face_backend);

    return &face->base;
}

static cairo_status_t
_cairo_aros_scaled_font_create_toy (cairo_toy_font_face_t       *toy_face,
                                    const cairo_matrix_t        *font_matrix,
                                    const cairo_matrix_t        *ctm,
                                    const cairo_font_options_t  *font_options,
                                    cairo_scaled_font_t        **font_out)
{
    cairo_font_face_t *face;
    cairo_scaled_font_t *font;
    cairo_status_t status;

    face = cairo_aros_font_face_create (toy_face->family, toy_face->slant, toy_face->weight);
    if (face->status)
        return face->status;

    status = _cairo_aros_font_face_scaled_font_create (face, font_matrix, ctm, font_options, &font);
    if (status) {
        cairo_font_face_destroy (face);
        return status;
    }

    cairo_font_face_destroy (face);

    *font_out = font;
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_aros_scaled_font_fini (void *abstract_font)
{
    cairo_aros_scaled_font_t *font = abstract_font;

    if (font == NULL)
        return;

    if (font->own_engine)
        _release_engine (font->engine);

    font->engine = NULL;
    font->own_engine = FALSE;
}

static cairo_status_t
_cairo_aros_scaled_font_glyph_init_metrics (cairo_aros_scaled_font_t *font,
                                            cairo_scaled_glyph_t     *glyph,
                                            struct GlyphMap          *gm)
{
    cairo_text_extents_t extents;

    if (gm == NULL) {
        extents.x_bearing = 0;
        extents.y_bearing = 0;
        extents.width = 0;
        extents.height = 0;
        extents.x_advance = font->xspace;
        extents.y_bearing = 0;
    }

    else {
        extents.x_bearing = gm->glm_X0;
        extents.y_bearing = -gm->glm_Y0;
        extents.width = gm->glm_BlackWidth;
        extents.height = gm->glm_BlackHeight;
        extents.x_advance = FIXEDTOFLOAT(gm->glm_Width);
        extents.y_advance = 0;
    }

    _cairo_scaled_glyph_set_metrics (glyph, &font->base, &extents);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_aros_scaled_font_glyph_init_surface (cairo_aros_scaled_font_t *font,
                                            cairo_scaled_glyph_t     *glyph,
                                            struct GlyphMap          *gm)
{
    cairo_image_surface_t *surface;
    UBYTE *src;
    unsigned char *dest;
    int sx, sy, dx, dy;

    if (gm == NULL) {
        surface = (cairo_image_surface_t *) cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
        if (surface->base.status)
            return surface->base.status;

        _cairo_scaled_glyph_set_surface (glyph, &font->base, surface);

        return CAIRO_STATUS_SUCCESS;
    }

    surface = (cairo_image_surface_t *) cairo_image_surface_create (CAIRO_FORMAT_A1,
                                                                    gm->glm_BlackWidth,
                                                                    gm->glm_BlackHeight);
    if (surface->base.status)
        return surface->base.status;

    sy = gm->glm_BlackTop;
    dy = 0;

    while (dy < gm->glm_BlackHeight) {
        sx = gm->glm_BlackLeft;
        dx = 0;

        src = gm->glm_BitMap + (gm->glm_BMModulo * sy) + sx;
        dest = surface->data + (surface->stride * dy);

        while (dx < (gm->glm_BlackWidth+7) >> 3) {
            dest[dx] = AROS_SWAP_BITS_BYTE (src[sx]);
            sx++; dx++;
        }

        sy++; dy++;
    }

    cairo_surface_set_device_offset (&surface->base, 0, gm->glm_Y0);

    _cairo_scaled_glyph_set_surface (glyph, &font->base, surface);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_aros_scaled_font_glyph_init_surface_a8 (cairo_aros_scaled_font_t *font,
                                               cairo_scaled_glyph_t     *glyph,
                                               struct GlyphMap          *gm)
{
    cairo_image_surface_t *surface;
    UBYTE *src;
    unsigned char *dest;
    int sx, sy, dx, dy;

    if (gm == NULL) {
        surface = (cairo_image_surface_t *) cairo_image_surface_create (CAIRO_FORMAT_A8, 1, 1);
        if (surface->base.status)
            return surface->base.status;

        _cairo_scaled_glyph_set_surface (glyph, &font->base, surface);

        return CAIRO_STATUS_SUCCESS;
    }

    surface = (cairo_image_surface_t *) cairo_image_surface_create (CAIRO_FORMAT_A8,
                                                                    gm->glm_BlackWidth,
                                                                    gm->glm_BlackHeight);
    if (surface->base.status)
        return surface->base.status;

    sy = gm->glm_BlackTop;
    dy = 0;

    while (dy < gm->glm_BlackHeight) {
        sx = gm->glm_BlackLeft;
        dx = 0;

        src = gm->glm_BitMap + (gm->glm_BMModulo * sy) + sx;
        dest = surface->data + (surface->stride * dy);

        while (dx < gm->glm_BlackWidth) {
            dest[dx] = src[sx];
            sx++; dx++;
        }

        sy++; dy++;
    }

    cairo_surface_set_device_offset (&surface->base, 0, gm->glm_Y0);

    _cairo_scaled_glyph_set_surface (glyph, &font->base, surface);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_aros_scaled_font_glyph_init (void                      *abstract_font,
                                    cairo_scaled_glyph_t      *glyph,
                                    cairo_scaled_glyph_info_t  info)
{
    cairo_aros_scaled_font_t *font = abstract_font;
    struct GlyphMap *gm;
    cairo_status_t status;

    if (info & CAIRO_SCALED_GLYPH_INFO_PATH)
        return CAIRO_INT_STATUS_UNSUPPORTED;

    gm = _get_glyph_map (font, _cairo_scaled_glyph_index (glyph));

    if (info & CAIRO_SCALED_GLYPH_INFO_METRICS) {
        status = _cairo_aros_scaled_font_glyph_init_metrics (font, glyph, gm);
        if (status) {
            _release_glyph_map (font, gm);
            return status;
        }
    }

    if (info & CAIRO_SCALED_GLYPH_INFO_SURFACE) {
        if (font->antialias)
            status = _cairo_aros_scaled_font_glyph_init_surface_a8 (font, glyph, gm);
        else
            status = _cairo_aros_scaled_font_glyph_init_surface (font, glyph, gm);

        if (status) {
            _release_glyph_map (font, gm);
            return status;
        }
    }

    _release_glyph_map (font, gm);
    return CAIRO_STATUS_SUCCESS;
}

static uint32_t
_get_kern (cairo_aros_scaled_font_t *font,
           uint16_t                  code,
           uint16_t                  code2)
{
    struct Library *BulletBase = font->engine->gle_Library;
    uint32_t kern;

    SetInfo (font->engine, OT_GlyphCode,  code,
                           OT_GlyphCode2, code2,
                           TAG_DONE);
    ObtainInfo (font->engine, OT_TextKernPair, (IPTR) &kern, TAG_DONE);

    return kern;
}

static cairo_int_status_t
_cairo_aros_scaled_font_text_to_glyphs (void           *abstract_font,
                                        double          x,
                                        double          y,
                                        const char     *utf8,
                                        cairo_glyph_t **glyphs,
                                        int            *num_glyphs)
{
    cairo_aros_scaled_font_t *font = abstract_font;
    uint16_t *utf16;
    int n16, s, d;
    cairo_status_t status;
    struct GlyphMap *gm;

    status = _cairo_utf8_to_utf16 ((const unsigned char *) utf8, -1, &utf16, &n16);
    if (status)
        return status;

    *glyphs = _cairo_malloc_ab (n16, sizeof (cairo_glyph_t));

    s = d = 0;
    while (s < n16) {
        gm = _get_glyph_map (font, utf16[s]);
        if (gm == NULL) {
            x += font->xspace;
            s++;
            continue;
        }

        if (s > 0) {
            x -= gm->glm_X0;
            x -= FIXEDTOFLOAT (_get_kern (font, utf16[s-1], utf16[s]));
        }

        (*glyphs)[d].index = utf16[s];
        (*glyphs)[d].x = x;
        (*glyphs)[d].y = y;

        x += gm->glm_X1 - gm->glm_X0;

        _release_glyph_map (font, gm);

        s++; d++;
    }

    free (utf16);

    *num_glyphs = d;

    return CAIRO_STATUS_SUCCESS;
}

const cairo_scaled_font_backend_t cairo_aros_scaled_font_backend = {
    CAIRO_FONT_TYPE_AROS,
    _cairo_aros_scaled_font_create_toy,
    _cairo_aros_scaled_font_fini,
    _cairo_aros_scaled_font_glyph_init,
    _cairo_aros_scaled_font_text_to_glyphs,
    NULL, /* ucs4_to_index */
    NULL, /* show_glyphs */
    NULL, /* load_truetype_table */
    NULL, /* map_glyphs_to_unicode */
};


