/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function UnlockLayerRom()
    Lang: english
*/
#include <proto/layers.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, UnlockLayerRom,

/*  SYNOPSIS */
	AROS_LHA(struct Layer *, l,   A5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 73, Graphics)

/*  FUNCTION
        Unlocks a prevously locked layer for access by other applications
        or intuition itself.
        If a task has locked a layer multiple times it must unlock it
        as many times as well as locks nest.
        This functions does the same as layers/UnlockLayerRom()

    INPUTS
	l - pointer to layer structure

    RESULT

    NOTES

    EXAMPLE

    BUGS
        Does not save all registers.

    SEE ALSO
        LockLayerRom() LockLayer() UnLockLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  ReleaseSemaphore(&l->Lock);

  AROS_LIBFUNC_EXIT
} /* UnlockLayerRom */
