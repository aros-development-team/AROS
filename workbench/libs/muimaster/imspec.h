/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_IMSPEC_H
#define _MUI_IMSPEC_H

char *zune_image_spec_duplicate(IPTR in);
void zune_image_spec_free(char *spec);

struct MUI_ImageSpec *zune_image_spec_to_structure(IPTR in, Object *obj);
void zune_image_spec_parse_string (STRPTR s, struct MUI_ImageSpec **out);
STRPTR zune_imspec_to_string (struct MUI_ImageSpec *spec);
struct MUI_ImageSpec *zune_get_pattern_spec(LONG muiipatt);
struct MUI_ImageSpec *zune_get_muipen_spec(LONG muipen);
BOOL __zune_imspec_set_state (struct MUI_ImageSpec *img, ULONG state);
LONG zune_imspec_get_width (struct MUI_ImageSpec *img);
LONG zune_imspec_get_height (struct MUI_ImageSpec *img);
void zune_imspec_set_width (struct MUI_ImageSpec *img, LONG w);
void zune_imspec_set_height (struct MUI_ImageSpec *img, LONG w);
void zune_imspec_set_scaled_size (struct MUI_ImageSpec *img, LONG w, LONG h);
//struct MUI_ImageSpec *zune_imspec_copy(struct MUI_ImageSpec *spec);
void zune_imspec_free(struct MUI_ImageSpec *spec);
void zune_imspec_setup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri);
void zune_imspec_cleanup(struct MUI_ImageSpec **spec, struct MUI_RenderInfo *mri);
void zune_imspec_show(struct MUI_ImageSpec *spec, Object *obj);
void zune_imspec_hide(struct MUI_ImageSpec *spec);
void zune_draw_image (struct MUI_RenderInfo *mri, struct MUI_ImageSpec *img,
		 LONG left, LONG top, LONG width, LONG height,
		 LONG xoffset, LONG yoffset, LONG flags);

/****/


int zune_imspec_get_minwidth(struct MUI_ImageSpec *spec);
int zune_imspec_get_minheight(struct MUI_ImageSpec *spec);

struct MUI_ImageSpec *zune_imspec_link_new(struct MUI_ImageSpec *linked);

void zune_link_rebind (struct MUI_ImageSpec *img, struct MUI_ImageSpec *new_link);

/***************/

/* Bitnums used for the flags for zune_draw_image() */
#define IMSPEC_SELECTED (0)

/* Flags for zune_draw_image() */
#define IMSPECF_SELECTED (1<<IMSPEC_SELECTED) 

#endif
