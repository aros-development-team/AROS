/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <hardware/blit.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

	AROS_LH2(void, SwapBitsRastPortClipRect,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct ClipRect *, cr, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 21, Layers)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    D(bug("SwapBitsRastPortClipRect(rp @ $%lx, cr @ $%lx)\n", rp, cr));

    BltRPtoCR(rp, cr, A_XOR_C, LayersBase);
    BltCRtoRP(rp, cr, A_XOR_C, LayersBase);
    BltRPtoCR(rp, cr, A_XOR_C, LayersBase);

    AROS_LIBFUNC_EXIT
} /* SwapBitsRastPortClipRect */
