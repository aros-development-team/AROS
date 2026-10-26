#ifndef GRAPHICS_LAYERSEXT_H
#define GRAPHICS_LAYERSEXT_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
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
/* Composited layers: the layer gets a surface bitmap of its own and the
   display composes it onto the screen (see LAYERCOMPOSITED in clip.h).
   Ignored, giving an ordinary smart refresh layer, when the screen bitmap's
   display cannot present surfaces. */
#define LA_Composited	(LA_AROS + 5)	/* BOOL. Default is FALSE */
#define LA_Opacity	(LA_AROS + 6)	/* 0..255 constant alpha, 255 = opaque (default) */
#define LA_AlphaSurface	(LA_AROS + 7)	/* BOOL. Surface with per pixel alpha. Default is FALSE */
#define LA_InputShape	(LA_AROS + 8)	/* struct Region *. Default is NULL (input where the layer is seen) */
/* LA_InputShape: where the layer takes input, apart from where it is seen.
   Layer relative, as LA_ShapeRegion; layers.library keeps a copy of its own
   (the caller's region may be disposed after the call). The point must
   also lie inside the layer's visible shape; inside the input shape the
   pixels' alpha does not matter, outside it the point goes to the layers
   behind. An empty region takes no input at all; NULL (the default) keeps
   the visible shape and, for an alpha surface, alpha 0 as the limits (see
   IsLayerHitAt()). It is kept when the layer is moved or sized; change it
   with ChangeLayerInputShape(). Works the same on composited and ordinary
   layers. */
/* An LA_AlphaSurface surface has the screen's pixel layout plus an alpha
   byte and starts fully transparent. Pens are mapped for the screen, so pen
   rendering leaves alpha 0; WritePixelArray() with RECTFMT_ARGB sets it.
   On screens whose layout has no spare byte (15/16/24 bit, LUT) the tag is
   ignored and the layer gets a constant opacity surface. */
/* Memory: the surfaces of all composited layers of a screen together may
   take at most four times the screen bitmap (layers.library's
   LAYERS_SURFACE_BUDGET_FACTOR), and a surface is never allocated when it
   would leave less than 4 MB of the largest free memory block. A layer that
   does not fit is created as an ordinary smart refresh layer (no
   LAYERCOMPOSITED in Layer.Flags, LA_Opacity and LA_AlphaSurface ignored),
   exactly as if LA_Composited had not been given, and a composited layer
   whose growth (SizeLayer(), MoveSizeLayer()) does not fit is converted
   into one, keeping its content. Check Layer.Flags for LAYERCOMPOSITED
   after creation and after resizing when it matters. */

#endif /* GRAPHICS_LAYERSEXT_H */
