/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <aros/libcall.h>
#include <proto/graphics.h>
#include <graphics/scale.h>

#include "layers_intern.h"
#include "basicfuncs.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

struct ScaleLayerParam
{
  struct TagItem    * taglist;
  struct LayersBase * LayersBase;
  BOOL                scaled;
};

AROS_UFP3(BOOL, ScaleLayerCallback,
   AROS_UFPA(struct Hook         *, hook, A0),
   AROS_UFPA(struct Layer        *, l   , A2),
   AROS_UFPA(struct ShapeHookMsg *, msg , A1));

/*****************************************************************************

    NAME */
#include <proto/layers.h>

        AROS_LH2(ULONG, ScaleLayer,

/*  SYNOPSIS */
        AROS_LHA(struct Layer   *, l         , A0),
        AROS_LHA(struct TagItem *, taglist   , A1),

/*  LOCATION */
        struct LayersBase *, LayersBase, 38, Layers)

/*  FUNCTION
        Scale the pixel content of the given layer. The layer's bounds and
        shape are left alone; the source rectangle of the layer's content
        is scaled into the destination rectangle and the result becomes the
        new content of the layer. Areas of the layer not covered by the
        destination rectangle are cleared.

    INPUTS
       l          - pointer to layer
       taglist    - LA_SRCX, LA_SRCY, LA_SRCWIDTH, LA_SRCHEIGHT
                    source rectangle in layer coordinates
                    (default: the whole layer)
                    LA_DESTX, LA_DESTY, LA_DESTWIDTH, LA_DESTHEIGHT
                    destination rectangle in layer coordinates
                    (default: the whole layer)

    RESULT
       TRUE if the content was scaled, FALSE otherwise.

    NOTES
       Only smart refresh layers can be scaled: simple refresh layers keep
       no copy of their pixels, and superbitmap layers own their pixels
       in the caller's bitmap.

    EXAMPLE

    BUGS

    SEE ALSO
       ChangeLayerShape(), graphics.library/BitMapScale()

    INTERNALS
       Implemented as a ChangeLayerShape() callback. When the callback runs
       the whole layer has been backed up into its ClipRect bitmaps. These
       are consolidated into one layer sized bitmap, scaled into a fresh
       bitmap, and that bitmap is installed as the layer's single hidden
       ClipRect. ChangeLayerShape() then shows the layer again from it.

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct ScaleLayerParam parm = {taglist, LayersBase, FALSE};
  struct Hook hook;

  if (!IS_SMARTREFRESH(l))
    return FALSE;

  hook.h_Entry = (HOOKFUNC)ScaleLayerCallback;
  hook.h_Data  = (APTR)&parm;

  /*
   * The callback keeps the current shape region, so the returned
   * (previous) shape is still installed and must not be freed here.
   */
  ChangeLayerShape(l, NULL, &hook);

  return parm.scaled;

  AROS_LIBFUNC_EXIT
} /* ScaleLayer */


/*
 * The ScaleLayer callback is doing the actual work of
 * scaling the layer.
 */
AROS_UFH3(BOOL, ScaleLayerCallback,
   AROS_UFHA(struct Hook         *, hook, A0),
   AROS_UFHA(struct Layer        *, l   , A2),
   AROS_UFHA(struct ShapeHookMsg *, msg , A1))
{
  AROS_USERFUNC_INIT

  struct ScaleLayerParam * slp = (struct ScaleLayerParam *)hook->h_Data;
  struct LayersBase * LayersBase = slp->LayersBase;
  struct TagItem * taglist = slp->taglist;
  struct BitMap * display_bm = l->rp->BitMap;
  struct BitMap * srcbm, * destbm;
  struct ClipRect * cr, * newcr;
  struct BitScaleArgs bsa;
  ULONG depth = GetBitMapAttr(display_bm, BMA_DEPTH);
  WORD  alignoff = ALIGN_OFFSET(l->bounds.MinX);
  LONG  destx, desty, destw, desth, bmw, bmh;

  /*
   * ChangeLayerShape() has backed the layer up: every ClipRect is hidden
   * and, for a smart refresh layer, carries a bitmap. There may be more
   * than one of them, so first gather them into a single bitmap laid out
   * like a ClipRect bitmap covering the whole layer.
   */
  if (NULL == l->ClipRect)
    return FALSE;

  srcbm = AllocBitMap(l->Width + ALIGN_CLIPRECT, l->Height, depth, BMF_CLEAR, display_bm);
  if (NULL == srcbm)
    return FALSE;

  for (cr = l->ClipRect; NULL != cr; cr = cr->Next)
  {
    if (NULL != cr->BitMap)
    {
      BltBitMap(cr->BitMap,
                ALIGN_OFFSET(cr->bounds.MinX),
                0,
                srcbm,
                cr->bounds.MinX - l->bounds.MinX + alignoff,
                cr->bounds.MinY - l->bounds.MinY,
                cr->bounds.MaxX - cr->bounds.MinX + 1,
                cr->bounds.MaxY - cr->bounds.MinY + 1,
                0x0c0,
                0xff,
                NULL);
    }
  }

  /*
   * The destination bitmap must at least cover the layer (it becomes the
   * layer's ClipRect bitmap) and the destination rectangle.
   */
  destx = GetTagData(LA_DESTX     , 0        , taglist);
  desty = GetTagData(LA_DESTY     , 0        , taglist);
  destw = GetTagData(LA_DESTWIDTH , l->Width , taglist);
  desth = GetTagData(LA_DESTHEIGHT, l->Height, taglist);

  bmw = MAX(l->Width , destx + destw);
  bmh = MAX(l->Height, desty + desth);

  destbm = AllocBitMap(bmw + ALIGN_CLIPRECT, bmh, depth, BMF_CLEAR, display_bm);
  if (NULL == destbm)
  {
    FreeBitMap(srcbm);
    return FALSE;
  }

  bsa.bsa_SrcX        = alignoff + GetTagData(LA_SRCX, 0, taglist);
  bsa.bsa_SrcY        = GetTagData(LA_SRCY, 0, taglist);
  bsa.bsa_SrcWidth    = GetTagData(LA_SRCWIDTH , l->Width , taglist);
  bsa.bsa_SrcHeight   = GetTagData(LA_SRCHEIGHT, l->Height, taglist);
  bsa.bsa_XSrcFactor  = bsa.bsa_SrcWidth;
  bsa.bsa_YSrcFactor  = bsa.bsa_SrcHeight;
  bsa.bsa_DestX       = alignoff + destx;
  bsa.bsa_DestY       = desty;
  bsa.bsa_XDestFactor = destw;
  bsa.bsa_YDestFactor = desth;
  bsa.bsa_SrcBitMap   = srcbm;
  bsa.bsa_DestBitMap  = destbm;
  bsa.bsa_Flags       = 0;
  bsa.bsa_XDDA        = 0;
  bsa.bsa_YDDA        = 0;
  bsa.bsa_Reserved1   = 0;
  bsa.bsa_Reserved2   = 0;

  BitMapScale(&bsa);

  FreeBitMap(srcbm);

  /*
   * Replace the layer's ClipRect list with one hidden ClipRect holding the
   * scaled content. Get the new ClipRect before freeing the old list so
   * that a failure leaves the layer untouched.
   */
  newcr = _AllocClipRect(l, LayersBase);
  if (NULL == newcr)
  {
    FreeBitMap(destbm);
    return FALSE;
  }

  _FreeClipRectListBM(l, l->ClipRect, LayersBase);

  newcr->bounds = l->bounds;
  newcr->Next   = NULL;
  newcr->lobs   = (struct Layer *)(IPTR)TRUE; /* hidden */
  newcr->BitMap = destbm;
  l->ClipRect   = newcr;

  slp->scaled = TRUE;

  /* Keep the current shape region (msg->NewShape == l->shaperegion) */
  return TRUE;

  AROS_USERFUNC_EXIT
}
