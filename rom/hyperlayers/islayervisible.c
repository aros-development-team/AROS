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
	AROS_LH1(LONG, IsLayerVisible,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l      , A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 40, Layers)

/*  FUNCTION
       Checks whether the layer is visible or not.

    INPUTS
       L       - pointer to layer 

    RESULT
       TRUE  - layer is visible
       FALSE - layer is invisible
  
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

  return IS_VISIBLE(l);

  AROS_LIBFUNC_EXIT
} /* ChangeLayerVisibility */
