/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/alib.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH1(void, InitLayers,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),

/*  LOCATION */
	struct LayersBase *, LayersBase, 5, Layers)

/*  FUNCTION
	Initializes the supplied Layer_Info, so it's ready for use.
	Leaves the Layer_Info in an unlocked state.

    INPUTS
	li -- pointer to Layer_Info structure

    RESULT

    NOTES
	This function is obsolete. Use NewLayerInfo() instead.

    EXAMPLE

    BUGS

    SEE ALSO
	NewLayerInfo()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    UWORD i;

    D(bug("InitLayers(li @ $%lx)\n", li));

    /* init LayerInfo structure with all zeros */
    for(i = 0; i < sizeof(struct Layer_Info); i++)
	((UBYTE *)li)[i] = 0;

    NewList((struct List *)&li->gs_Head);

    InitSemaphore(&li->Lock);

    li->fatten_count = -1;
    li->Flags = LIFLG_SUPPORTS_OFFSCREEN_LAYERS;
    
    AROS_LIBFUNC_EXIT
} /* InitLayers */
