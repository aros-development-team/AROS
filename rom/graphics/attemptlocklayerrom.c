/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AttemptLockLayerRom()
    Lang: english
*/
#include <proto/layers.h>
#include <graphics/clip.h>
#include <proto/exec.h>
#include <exec/types.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(BOOL, AttemptLockLayerRom,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l, A5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 109, Graphics)

/*  FUNCTION
        Try to lock the current layer. If it is already locked this
        function will return FALSE, TRUE otherwise.
        If the layer could be locked successfully nesting will take place
        which means that for every successful locking of a layer 
        UnlockLayerRom() has to be called for that layer to let other 
        tasks access that layer. 

    INPUTS
        l - pointer to layer

    RESULT
        TRUE  - layer is successfully locked for the task
        FALSE - layer could not be locked, it's locked by another task.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
      LockLayerRom() UnlockLayerRom(), 

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  return AttemptSemaphore(&l->Lock);

  AROS_LIBFUNC_EXIT
} /* AttemptLockLayerRom */
