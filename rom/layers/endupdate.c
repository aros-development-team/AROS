/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <graphics/clip.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <graphics/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"
#include <proto/layers.h>

/*****************************************************************************

    NAME */

	AROS_LH2(void, EndUpdate,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l    , A0),
	AROS_LHA(UWORD         , flag , D0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 14, Layers)

/*  FUNCTION
        After the damaged areas are update this routine should be
        called so the regular cliprects of the layer can be installed.
        

    INPUTS
        l    -  pointer to layer
        flag -  TRUE if the update was complete. The damage list is disposed.
                FALSE it the update was partial. The damage list is kept. 

    RESULT

    NOTES

    EXAMPLE

    BUGS
      not tested

    SEE ALSO
         BeginUpdate()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  struct ClipRect * FirstCR = l->ClipRect;
  struct ClipRect * CR;

  if (FALSE != flag)
  {
    /* the update was complete so I free the damage list */
    ClearRegion(l->DamageList);
  } 

  /* In any case I free the list of temporary cliprects that represent
     the damage list */

  CR = FirstCR;
  while (NULL != CR)
  {
    FirstCR = CR->Next;
    _FreeClipRect(CR, l);
    CR = FirstCR;
  }
  
  /* now I reinstall the regular ClipRects. */
  l->ClipRect = l->cr;

  l->Flags &= ~LAYERUPDATING;

  UnlockLayer(l);

  AROS_LIBFUNC_EXIT
} /* EndUpdate */
