/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <exec/types.h>

#include <proto/muimaster.h>
#include <proto/graphics.h>

#include <imspec.h>
/*  #include <gc.h> */
#include <prefs.h>
#include <file.h>
#include <areadata.h>
#include <pixmap.h>
#include <renderinfo.h>

/* FIXME : implement vector gfx */

#define patstipple_width 16
#define patstipple_height 16
static char patstipple_bits[] = {
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
    0x55, 0x55, 0xaa, 0xaa,
};

GdkColor __mpens[] = {
    {0, 0xffff, 0xffff, 0xffff}, /* MPEN_SHINE */
    {0, 0xd000, 0xd000, 0xd000}, /* MPEN_HALFSHINE */
    {0, 0xA000, 0xA000, 0xA000}, /* MPEN_BACKGROUND */
    {0, 0x5000, 0x5000, 0x5000}, /* MPEN_HALFSHADOW */
    {0, 0x0000, 0x0000, 0x0000}, /* MPEN_SHADOW */
    {0, 0x0000, 0x0000, 0x0000}, /* MPEN_TEXT */
    {0, 0x0500, 0x8400, 0xc400}, /* MPEN_FILL */
    {0, 0xf400, 0xb500, 0x8b00}, /* MPEN_MARK */
}; /* MPEN_COUNT */

static MPenCouple patternPens[] = {
    {MPEN_BACKGROUND, MPEN_BACKGROUND}, /* MUII_BACKGROUND */
    {MPEN_SHADOW, MPEN_SHADOW},         /* MUII_SHADOW */
    {MPEN_SHINE, MPEN_SHINE},           /* MUII_SHINE */
    {MPEN_FILL, MPEN_FILL},             /* MUII_FILL */
    {MPEN_SHADOW, MPEN_BACKGROUND},     /* MUII_SHADOWBACK */
    {MPEN_SHADOW, MPEN_FILL},           /* MUII_SHADOWFILL */
    {MPEN_SHADOW, MPEN_SHINE},          /* MUII_SHADOWSHINE */
    {MPEN_FILL, MPEN_BACKGROUND},       /* MUII_FILLBACK */
    {MPEN_FILL, MPEN_SHINE},            /* MUII_FILLSHINE */
    {MPEN_SHINE, MPEN_BACKGROUND},      /* MUII_SHINEBACK */
    {MPEN_FILL, MPEN_BACKGROUND},       /* MUII_FILLBACK2 */
    {MPEN_HALFSHINE, MPEN_BACKGROUND},  /* MUII_HSHINEBACK */
    {MPEN_HALFSHADOW, MPEN_BACKGROUND}, /* MUII_HSHADOWBACK */
    {MPEN_HALFSHINE, MPEN_SHINE},       /* MUII_HSHINESHINE */
    {MPEN_HALFSHADOW, MPEN_SHADOW},     /* MUII_HSHADOWSHADOW */
    {MPEN_MARK, MPEN_SHINE},            /* MUII_MARKSHINE */
    {MPEN_MARK, MPEN_HALFSHINE},        /* MUII_MARKHALFSHINE */
    {MPEN_MARK, MPEN_BACKGROUND},       /* MUII_MARKBACKGROUND */
};

#define PATTERN_COUNT (MUII_LASTPAT - MUII_BACKGROUND + 1)

struct MUI_ImageSpec *__patternSpec[PATTERN_COUNT];
struct MUI_ImageSpec *__penSpec[MPEN_COUNT];


/*******************************/

static void
__destroy_images (void)
{
    int i;

    for (i = 0; i < PATTERN_COUNT; i++)
    {
	zune_imspec_free(__patternSpec[i]);
    }
    for (i = 0; i < MPEN_COUNT; i++)
    {
	zune_imspec_free(__penSpec[i]);
    }
}

void
__zune_images_init(void)
{
    int i;

    g_atexit(__destroy_images);
    for (i = 0; i < PATTERN_COUNT; i++)
    {
	__patternSpec[i] = zune_imspec_pattern_new(MUII_BACKGROUND + i);
    }

    for (i = 0; i < MPEN_COUNT; i++)
    {
	__penSpec[i] = zune_imspec_muipen_new(i);
    }
}


struct MUI_ImageSpec *
zune_get_pattern_spec(LONG muiipatt)
{
    g_return_val_if_fail(_between(MUII_BACKGROUND, muiipatt, MUII_LASTPAT), NULL);
    return __patternSpec[muiipatt - MUII_BACKGROUND];
}

struct MUI_ImageSpec *
zune_get_muipen_spec (LONG muipen)
{
    g_return_val_if_fail(_between(MPEN_SHINE, muipen, MPEN_COUNT - 1), NULL);
    return __penSpec[muipen];
}


/****************************************************/

static void	
zune_render_set_pattern (struct MUI_RenderInfo *mri, LONG pattern)
{
    SetAPen(mri->mri_RastPort, mri->mri_Pens[patternPens[pattern - MUII_BACKGROUND].fg]);
    SetBPen(mri->mri_RastPort, mri->mri_Pens[patternPens[pattern - MUII_BACKGROUND].bg]);    
    gdk_gc_set_fill(mri->mri_RastPort, GDK_OPAQUE_STIPPLED);
    if (!mri->mri_PatternStipple)
    {
	mri->mri_PatternStipple =
	    gdk_bitmap_create_from_data(mri->mri_Window, patstipple_bits,
					patstipple_width, patstipple_height);
    }
    gdk_gc_set_stipple(mri->mri_RastPort, mri->mri_PatternStipple);
}

static void	
zune_render_set_muipen (struct MUI_RenderInfo *mri, MPen pen)
{
    SetAPen(mri->mri_RastPort, mri->mri_Pens[pen]);
}

static void	
zune_render_set_rgb (struct MUI_RenderInfo *mri, GdkColor *color)
{
    gdk_gc_set_foreground(mri->mri_RastPort, color);
}

static void
zune_penspec_set_mri (struct MUI_RenderInfo *mri, struct MUI_PenSpec *penspec)
{
    switch (penspec->ps_penType)
    {
	case PST_MUI:
	    zune_render_set_muipen(mri, penspec->ps_mui);
	    break;
	case PST_RGB:
/*  g_print("zune_penspec_set_mri gonna allocate color\n"); */
/*  	    penspec->ps_Type = PST_RGBALLOCATED; */
/*  	    gdk_colormap_alloc_color(mri->mri_Colormap, &penspec->u.rgb, FALSE, TRUE); */
	    zune_render_set_rgb(mri, &penspec->ps_rgbColor);
	    break;
	case PST_CMAP:
	    SetAPen(mri->mri_RastPort, penspec->ps_cmap);
	    break;
    }
}

/*
 * fill a rectangle with a preset MUI pattern
 */
void
_zune_fill_pattern_rectangle(struct MUI_ImageSpec *img,
			     struct MUI_RenderInfo *mri,
			     LONG left, LONG top, LONG width, LONG height,
			     LONG xoffset, LONG yoffset, LONG flags)
{
    zune_render_set_pattern(mri, img->u.pattern);
    gdk_draw_rectangle (mri->mri_Window, mri->mri_RastPort, TRUE,
			left, top, width, height);
    gdk_gc_set_fill(mri->mri_RastPort, GDK_SOLID);
}

void
_zune_fill_muipen_rectangle(struct MUI_ImageSpec *img,
			    struct MUI_RenderInfo *mri,
			    LONG left, LONG top, LONG width, LONG height,
			    LONG xoffset, LONG yoffset, LONG flags)
{
    zune_penspec_set_mri(mri, &img->u.pen);
    gdk_draw_rectangle (mri->mri_Window, mri->mri_RastPort, TRUE,
			left, top, width, height);
}

void
_zune_fill_rgb_rectangle(struct MUI_ImageSpec *img,
			 struct MUI_RenderInfo *mri,
			 LONG left, LONG top, LONG width, LONG height,
			 LONG xoffset, LONG yoffset, LONG flags)
{
    zune_penspec_set_mri(mri, &img->u.pen);
    gdk_draw_rectangle (mri->mri_Window, mri->mri_RastPort, TRUE,
			left, top, width, height);
}

void
_zune_fill_cmap_rectangle(struct MUI_ImageSpec *img,
			  struct MUI_RenderInfo *mri,
			  LONG left, LONG top, LONG width, LONG height,
			  LONG xoffset, LONG yoffset, LONG flags)
{
    zune_penspec_set_mri(mri, &img->u.pen);
    gdk_draw_rectangle (mri->mri_Window, mri->mri_RastPort, TRUE,
			left, top, width, height);
}


/* Tile a pixmap in a rectangle
 * Get pixmap/mask
 * pixmap as tile
 * mask as clipmask
 * set ts origin
 * draw rect
 * restore gc
 */
/* do not use clip mask; only one tile under mask was drawn */
void
_zune_fill_tiled_rectangle(struct MUI_ImageSpec *img,
			   struct MUI_RenderInfo *mri,
			   LONG left, LONG top, LONG width, LONG height,
			   LONG xoffset, LONG yoffset)
{
    GdkPixmap *pixmap;

    g_return_if_fail((pixmap = __zune_imspec_get_pixmap(img)) != NULL);

    gdk_gc_set_fill(mri->mri_RastPort, GDK_TILED);
    gdk_gc_set_tile(mri->mri_RastPort, pixmap);
    gdk_gc_set_ts_origin(mri->mri_RastPort, xoffset, yoffset);

    gdk_draw_rectangle (mri->mri_Window, mri->mri_RastPort, TRUE,
			left, top, width, height);
    gdk_gc_set_fill(mri->mri_RastPort, GDK_SOLID);
    gdk_gc_set_ts_origin(mri->mri_RastPort, 0, 0);
}


/* brush paint: get current pixmap/mask, and draw the part in the
 * rectangle bounds.
 */
void
_zune_fill_scaled_rectangle(struct MUI_ImageSpec *img,
			    struct MUI_RenderInfo *mri, 
			    LONG left, LONG top, LONG width, LONG height,
			    LONG xoffset, LONG yoffset)
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;

    g_return_if_fail((pixmap = __zune_imspec_get_pixmap(img)) != NULL);

    mask = __zune_imspec_get_mask(img);

    gdk_gc_set_clip_mask(mri->mri_RastPort, mask);
    gdk_gc_set_clip_origin(mri->mri_RastPort, xoffset, yoffset);
/*  g_print("draw pixmap: srcx=%d srcy=%d dstx=%d dsty=%d w=%d h=%d\n", */
/*  	left - xoffset, top - yoffset, left, top, width, height); */
    gdk_draw_pixmap (mri->mri_Window, mri->mri_RastPort, pixmap,
		     left - xoffset, top - yoffset, left, top, width, height);
    gdk_gc_set_clip_mask(mri->mri_RastPort, NULL);
}

void
_zune_fill_brush_rectangle(struct MUI_ImageSpec *img,
			   struct MUI_RenderInfo *mri, 
			   LONG left, LONG top, LONG width, LONG height,
			   LONG xoffset, LONG yoffset, LONG flags)
{
    _zune_fill_scaled_rectangle(img, mri, left, top, width, height,
				 xoffset, yoffset);
}

void
_zune_fill_vector_rectangle(struct MUI_ImageSpec *img,
			    struct MUI_RenderInfo *mri, 
			    LONG left, LONG top, LONG width, LONG height,
			    LONG xoffset, LONG yoffset, LONG flags)
{
}

/*
 * draw a tiled rectangle, with tile start at 0,0
 */
void
_zune_fill_bitmap_rectangle(struct MUI_ImageSpec *img,
			    struct MUI_RenderInfo *mri, 
			    LONG left, LONG top, LONG width, LONG height,
			    LONG xoffset, LONG yoffset, LONG flags)
{
    _zune_fill_tiled_rectangle(img, mri, left, top, width, height,
				0, 0);
}




