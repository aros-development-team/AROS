/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"
#include "basicfuncs.h"

	AROS_LH2(struct Hook *, InstallLayerInfoHook,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct Hook       *, hook, A1),

/*  LOCATION */
	struct LayersBase *, LayersBase, 34, Layers)

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

    struct Hook *OldHook;

    D(bug("InstallLayerInfoHook(li @ $%lx, hook @ $%lx)\n", li, hook));

    if(!_AccessLayerInfo(li, LayersBase))
	return (struct Hook *)~0;

    OldHook = li->BlankHook;

    li->BlankHook = hook;

    _ReleaseLayerInfo(li, LayersBase);

    return OldHook;

    AROS_LIBFUNC_EXIT
} /* InstallLayerInfoHook */
