/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <layers_intern.h>
#include <aros/libcall.h>
#include <proto/graphics.h>
#include "basicfuncs.h"

/*****************************************************************************

    NAME */
#include <proto/layers.h>
	AROS_LH2(struct Region *, ChangeLayerShape,

/*  SYNOPSIS */
	AROS_LHA(struct Layer  *, l        , A0),
	AROS_LHA(struct Region *, newshape , A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 41, Layers)

/*  FUNCTION
       Changes the shape of the layer. Once a layer is created
       it has a certain shape. This shape can be changed to
       any other arbitrary shape but the layer will always
       remember its initial shape which means that this
       function will never return the original shape's region.
       Only regions that were installed with this function
       will be returned by this function. These regions can then
       be discarded.
       When the shape of a layer is changed the pixel content
       is copied into the ClipRects of the original shape unless
       of course it is a simple refresh layer.
       Once this function is called with the parameter newshape = NULL
       it will reinstall the original shape of the layer. If anything
       was changed in the appearance of the layer in the newshape
       it will be visible then as well.

    INPUTS
       L        - pointer to layer 
       newshape - pointer to a region that comprises the new shape
                  of the layer. 

    RESULT
       The region structure of a previously installed region is
       returned. The region structure of the original region with
       which the layer was created is never returned!
  
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

  return NULL;

  AROS_LIBFUNC_EXIT
} /* ChangeLayerShape */
