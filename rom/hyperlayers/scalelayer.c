/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/types.h>
#include <exec/memory.h>
#include "layers_intern.h"
#include <aros/libcall.h>
#include <proto/graphics.h>
#include <graphics/scale.h>
#include "basicfuncs.h"


struct ScaleLayerParam
{
  struct TagItem * taglist;
  struct LayersBase * LayersBase;
};

AROS_UFP3(struct Region *, ScaleLayerCallback,
   AROS_UFPA(struct Hook                *, hook     , A0),
   AROS_UFPA(struct Layer               *, l        , A2),
   AROS_UFPA(struct ChangeLayerShapeMsg *, clsm     , A1));

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(ULONG, ScaleLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer   *, l         , A0),
	AROS_LHA(struct TagItem *, taglist   , A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 42, Layers)

/*  FUNCTION
        Scale the given layer. This function will use the
        current shape of the layer and resize it according to
        the given newwidth/newheight.

    INPUTS
       L           - pointer to layer
       newwidth    - new width of the layer
       newheight   - new height of the layer

    RESULT
       TRUE if everything went alright, FALSE otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  struct ScaleLayerParam parm = {taglist, LayersBase};
  struct Region * oldshape;
  struct Hook hook;
  
  hook.h_Entry = (HOOKFUNC)ScaleLayerCallback;
  hook.h_Data  = (APTR)&parm;

  oldshape = ChangeLayerShape(l, 0, &hook);

  /*
   * I must not free oldshape here since it is also the new shape!
   */

  return FALSE;
  AROS_LIBFUNC_EXIT
} /* ScaleLayer */







/*
 * The ScaleLayer callback is doing the actul work of
 * scaling the layer.
 */
AROS_UFH3(struct Region *, ScaleLayerCallback,
   AROS_UFHA(struct Hook                *, hook     , A0),
   AROS_UFHA(struct Layer               *, l        , A2),
   AROS_UFHA(struct ChangeLayerShapeMsg *, clsm     , A1))
{
  AROS_USERFUNC_INIT

  struct BitScaleArgs bsa;
  struct BitMap * bm = NULL;
  struct ScaleLayerParam * slp = (struct ScaleLayerParam *)hook->h_Data;
  struct LayersBase * LayersBase = slp->LayersBase;
  
  struct TagItem * taglist = slp->taglist;

  if (l->ClipRect->Next)
  {
    kprintf("%s: Only expecting one ClipRect - leaving!\n",__FUNCTION__);
    return NULL;
  }

  bm = AllocBitMap(GetTagData(LA_DESTWIDTH , l->Width , taglist),
                   GetTagData(LA_DESTHEIGHT, l->Height, taglist),
                   l->ClipRect->BitMap->Depth,
                   0,
                   l->rp->BitMap);

  bsa.bsa_SrcX        = GetTagData(LA_SRCX, 0, taglist);
  bsa.bsa_SrcY        = GetTagData(LA_SRCY, 0, taglist);
  bsa.bsa_SrcWidth    = GetTagData(LA_SRCWIDTH , l->Width , taglist);
  bsa.bsa_SrcHeight   = GetTagData(LA_SRCHEIGHT, l->Height, taglist);
  bsa.bsa_XSrcFactor  = bsa.bsa_SrcWidth;
  bsa.bsa_YSrcFactor  = bsa.bsa_SrcHeight;
  bsa.bsa_DestX       = GetTagData(LA_DESTX, 0, taglist);
  bsa.bsa_DestY       = GetTagData(LA_DESTY, 0, taglist);
/*
  bsa.bsa_DestWidth   = GetTagData(LA_DESTWIDTH , l->Width , taglist);
  bsa.bsa_DestHeight  = GetTagData(LA_DESTHEIGHT, l->Height, taglist);
 */
  bsa.bsa_XDestFactor = GetTagData(LA_DESTWIDTH , l->Width , taglist);
  bsa.bsa_YDestFactor = GetTagData(LA_DESTHEIGHT, l->Height, taglist);
  bsa.bsa_SrcBitMap   = l->ClipRect->BitMap;
  bsa.bsa_DestBitMap  = bm;
  bsa.bsa_Flags       = 0;
#if 0
  bsa.bsa_XDDA;
  bsa.bsa_YDDA;
  bsa.bsa_Reserved1;
  bsa.bsa_Reserved2;
#endif

//kprintf("Scaling bitmap!\n");
  BitMapScale(&bsa);

  FreeBitMap(l->ClipRect->BitMap);
  l->ClipRect->BitMap = bm;
//kprintf("Leaving %s!\n",__FUNCTION__);

kprintf("shaperegion: %p\n",l->shaperegion);

  return l->shaperegion;

  AROS_USERFUNC_EXIT
}
