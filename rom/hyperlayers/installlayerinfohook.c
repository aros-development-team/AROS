/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
        Install a backfill hook into the LayerInfo structure. This
        backfill hook will be called to clear the areas where there
        is no layer. It will be used for every layer.        

    INPUTS
        li - pointer to layer info

    RESULT
        If there was a backfill hook installed before it will be 
        returned. LAYERS_BACKFILL will be returned if the default 
        backfill hook was installed, LAYERS_NOBACKFILL if there
        was nothing to be called for clearing an area.

    NOTES
        The hook is not called immediately. It will be called for
        those areas that have to be cleared when layers move away
        from those areas.

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

    struct Hook *OldHook;

    D(bug("InstallLayerInfoHook(li @ $%lx, hook @ $%lx)\n", li, hook));

    LockLayerInfo(li);    

    OldHook = li->BlankHook;

    li->BlankHook = hook;
    
    if (li->check_lp)
    {
    	LockLayer(0, li->check_lp);
	li->check_lp->BackFill = hook;
	UnlockLayer(li->check_lp);
    }
    
    UnlockLayerInfo(li);

    return OldHook;

    AROS_LIBFUNC_EXIT
} /* InstallLayerInfoHook */
