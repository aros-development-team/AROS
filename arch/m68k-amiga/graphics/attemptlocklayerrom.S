/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AttemptLockLayerRom()
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/graphics.h>

	AROS_LH1(BOOL, AttemptLockLayerRom,

    SYNOPSIS
	AROS_LHA(struct Layer *, l, A5),

    LOCATION
	struct GfxBase *, GfxBase, 109, Graphics)

    FUNCTION
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
      LockLayerRom(), UnlockLayerRom()

    INTERNALS

    HISTORY

*****************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(AttemptLockLayerRom,Graphics)

AROS_SLIB_ENTRY(AttemptLockLayerRom,Graphics):
    movem.l %d1/%a0-%a1/%a6,%sp@-
    move.l  SysBase,%a6
    lea.l   %a5@(ly_Lock),%a0
    jsr     %a6@(AttemptSemaphore)
    movem.l %sp@+,%d1/%a0-%a1/%a6
    rts
