/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function LockLayerRom()
    Lang: english
*/
#include <proto/layers.h>
#include <proto/exec.h>
#include <graphics/clip.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, LockLayerRom,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l,   A5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 72, Graphics)

/*  FUNCTION
        Locks the layer. Returns when the layer is locked for
        exclusive use.
        This call behaves like when a semaphore is locked. The
        same task may lock the same layer several times without
        locking itself out. For every call to this function a
        call to UnlockLayerRom() has to be made as the calls nest.
        This function will also prevent intuition from locking the
        layer so the layer should not be blocked too long.
        This function does exactly the same as layers/LockLayer()

    INPUTS
        l - pointer to layer that is to be locked

    RESULT

    NOTES

    EXAMPLE

    BUGS
       Does not save all registers.

    SEE ALSO
       UnlockLayerRom() LockLayer() UnLockLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  ObtainSemaphore(&l->Lock);

  AROS_LIBFUNC_EXIT
} /* LockLayerRom */
