#ifndef GRAPHICS_LAYERSEXT_H
#define GRAPHICS_LAYERSEXT_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layer extensions for the new AROS layers.library
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/* Tags for CreateLayerTagList */

/* AmigaOS4-compatible */
#define LA_ShapeRegion  (TAG_USER + 34)
#define LA_ShapeHook    (TAG_USER + 35) /* struct Region *. Default is NULL (rectangular shape) */
#define LA_InFrontOf    (TAG_USER + 36)
#define LA_Hidden	(TAG_USER + 41)	/* BOOL. Default is FALSE */

/* MorphOS-compatible */
#define LA_Dummy        (TAG_USER + 1024)
#define LA_BackfillHook (LA_Dummy + 1) /* struct Hook *. Default is LAYERS_BACKFILL */
#define LA_TransRegion  (LA_Dummy + 2) /* struct Region *. Default is NULL (rectangular shape) */
#define LA_TransHook    (LA_Dummy + 3)
#define LA_WindowPtr    (LA_Dummy + 4)
#define LA_SuperBitMap  (LA_Dummy + 5) /* struct BitMap *. Default is NULL (none) */

/* AROS-specific */
#define LA_AROS		(TAG_USER + 1234)
#define LA_Behind	(LA_AROS + 3)
#define LA_ChildOf	(LA_AROS + 4)

#endif /* GRAPHICS_LAYERSEXT_H */
