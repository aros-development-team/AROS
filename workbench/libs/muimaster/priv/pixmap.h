/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __PIXMAP_H__
#define __PIXMAP_H__

struct MUI_ImageSpec;

void
_zune_bitmap_load(struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri);

void
_zune_bitmap_unload(struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri);

void
_zune_brush_load(struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri);

void
_zune_brush_unload (struct MUI_ImageSpec *img, struct MUI_RenderInfo *mri);

void
_zune_brush_render (struct MUI_ImageSpec *img, Object *obj);

void
_zune_brush_unrender (struct MUI_ImageSpec *img);


#endif

