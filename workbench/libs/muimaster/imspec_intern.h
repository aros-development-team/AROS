/*
    Copyright © 2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef MUI_IMSPEC_INTERN_H
#define MUI_IMSPEC_INTERN_H

/* This header contains the definition of the internal ImageSpec
 * structure (ie. parsed, in memory).
 * It's private, used only by imspec implementation, and shouldnt
 * be included by other Zune components, which should refer to
 * the public imspec API in imspec.h
 */

#ifndef LIBRARIES_MUI_H
#include "mui.h"
#endif

#include "muimaster_intern.h"
#include "penspec.h"

typedef enum {
    IST_PATTERN,  /* "0:%ld" = a dithered mix of MUI pens */
    IST_VECTOR,   /* "1:%ld" = builtin code to draw vector image */
    IST_COLOR,    /* "2:" + PenSpec = a pen */
    IST_BOOPSI,   /* "3:%s" = boopsi image class */
    IST_BRUSH,    /* "3:%s", "4:%s" = small brushes */
    IST_BITMAP,   /* "5:%s" = a picture to tile in background */
    IST_CONFIG,   /* "6:%ld" = a configured image/background (indirection) */
    IST_SCALED_GRADIENT, /* "7:([Hh]|[Vv]|angle),<IST_COLOR>-<IST_COLOR>" = a scaled gradient */
    IST_TILED_GRADIENT, /* "8:([Hh]|[Vv]|angle),<IST_COLOR>-<IST_COLOR>" = a tiled gradient */
} ImageSpecType;

#define CHECKBOX_IMAGE 4

#if 0
enum MUI_ImageSpec_Flags {
    IMF_SETUP = 1, /* struct is between _setup and _cleanup */
    IMF_SHOW = 2, /* struct is between _show and _hide */
};
#endif

struct MUI_ImageSpec_intern;

typedef void (*VECTOR_DRAW_FUNC)(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height, LONG state);

/* should really contain an union */
struct MUI_ImageSpec_intern /* _intern */
{
    ImageSpecType type;
/*      UWORD flags;          */         /* see MUI_ImageSpec_Flags */
/*      struct MUI_RenderInfo *mri; */
    union {
	/* IST_PATTERN */
	LONG pattern;
	/* IST_VECTOR */
	struct {
	    LONG type;
	    VECTOR_DRAW_FUNC draw;
	} vect;
	/* IST_COLOR */
	struct MUI_PenSpec_intern penspec;
	/* IST_BOOPSI */
	struct {
	    CONST_STRPTR filename;
	    Object *obj;
	} boopsi;
	/* IST_BRUSH */
	struct {
	    CONST_STRPTR filename[2];
	    struct dt_node *dt[2];
	} brush;
	/* IST_BITMAP */
	struct {
	    CONST_STRPTR filename;
	    struct dt_node *dt;
	} bitmap;
	/* IST_CONFIG */
	struct {
	    LONG muiimg;                           /* index in prefs->imagespecs[] */
	} cfg;
	struct {
	    struct MUI_RenderInfo *mri;
            UWORD angle;                     /* integer values in [0,360) */
            Object *obj;
            ULONG start_rgb[3];
            ULONG end_rgb[3];
	    ULONG start_pen;
	    ULONG end_pen;
	    BOOL  start_pen_is_allocated;
	    BOOL  end_pen_is_allocated;
	} gradient;
    } u;

};

struct MUI_ImageSpec_intern *zune_imspec_create_vector(LONG vect);
BOOL zune_imspec_vector_get_minmax(struct MUI_ImageSpec_intern *spec, struct MUI_MinMax *minmax);

VOID zune_scaled_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
                                    STRPTR buf);
VOID zune_tiled_gradient_intern_to_string(struct MUI_ImageSpec_intern *spec,
                                    STRPTR buf);
BOOL zune_gradient_string_to_intern(CONST_STRPTR str,
                                     struct MUI_ImageSpec_intern *spec);
VOID zune_gradient_draw
(
    struct MUI_ImageSpec_intern *spec,
    struct MUI_RenderInfo *mri,
    WORD x1, WORD y1, WORD x2, WORD y2,
    WORD xoff, WORD yoff
);

BOOL zune_gradientspec_setup(struct MUI_ImageSpec_intern *spec, struct MUI_RenderInfo *mri);
VOID zune_gradientspec_cleanup(struct MUI_ImageSpec_intern *spec);

#endif
