#ifndef GRAPHICS_LAYERSEXT_H
#define GRAPHICS_LAYERSEXT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layer extensions for the new AROS layers.library
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define LA_Dummy	(TAG_USER + 1234)

/* Tags for CreateLayerTagList */

#define LA_Type		(LA_Dummy + 1) /* LAYERSIMPLE, LAYERSMART (default) -or LAYERSUPER */
#define LA_Priority	(LA_Dummy + 2) /* -128 .. 127 or LPRI_NORMAL (default) or LPRI_BACKDROP */
#define LA_Behind	(LA_Dummy + 3) /* BOOL. Default is FALSE */
#define LA_Invisible	(LA_Dummy + 4) /* BOOL. Default is FALSE */
#define LA_BackFill	(LA_Dummy + 5) /* struct Hook *. Default is LAYERS_BACKFILL */
#define LA_SuperBitMap	(LA_Dummy + 6) /* struct BitMap *. Default is NULL (none) */
#define LA_Shape	(LA_Dummy + 7) /* struct Region *. Default is NULL (rectangular shape) */

#define LPRI_NORMAL 	0
#define LPRI_BACKDROP	-50

#endif /* GRAPHICS_LAYERSEXT_H */
