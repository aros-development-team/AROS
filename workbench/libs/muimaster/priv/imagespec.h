/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_IMAGESPEC_H__
#define __ZUNE_IMAGESPEC_H__

#include <intuition/classes.h>
#include <intuition/imageclass.h>

/* This header may be included by all Zune modules.
 * Here are drawing functions and public imspec functions.
 * Definitions and additional private declarations are in imspec.[ch]
 * By private, means restricted to the few modules that need to know.
 * Currently: image_gdk.c, pixmap_imlib.c, imspec.c
 */

struct MUI_AreaData;
struct MUI_RenderInfo;
struct MUI_ImageSpec;

/*****************/

struct MUI_ImageSpec *
zune_image_spec_to_structure (ULONG in);

void
zune_image_spec_parse_string (STRPTR s, struct MUI_ImageSpec **out);

STRPTR
zune_imspec_to_string (struct MUI_ImageSpec *spec);

struct MUI_ImageSpec *
zune_get_pattern_spec(LONG muiipatt);

struct MUI_ImageSpec *
zune_get_muipen_spec(LONG muipen);

BOOL
__zune_imspec_set_state (struct MUI_ImageSpec *img, ULONG state);

LONG
zune_imspec_get_width (struct MUI_ImageSpec *img);

LONG
zune_imspec_get_height (struct MUI_ImageSpec *img);

void
zune_imspec_set_width (struct MUI_ImageSpec *img, LONG w);

void
zune_imspec_set_height (struct MUI_ImageSpec *img, LONG w);

void
zune_imspec_set_scaled_size (struct MUI_ImageSpec *img, LONG w, LONG h);

struct MUI_ImageSpec *
zune_imspec_copy(struct MUI_ImageSpec *spec);

void
zune_imspec_free(struct MUI_ImageSpec *spec);

void
zune_imspec_setup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri);

void
zune_imspec_cleanup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri);

void
zune_imspec_show(struct MUI_ImageSpec *spec, Object *obj);

void
zune_imspec_hide(struct MUI_ImageSpec *spec);

/****/

struct MUI_ImageSpec *
zune_imspec_link_new(struct MUI_ImageSpec *linked);

void
zune_link_rebind (struct MUI_ImageSpec *img, struct MUI_ImageSpec *new_link);

/***************/


#endif

