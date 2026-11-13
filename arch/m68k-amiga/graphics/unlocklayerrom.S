/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function UnlockLayerRom()
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/graphics.h>

	AROS_LH1(void, UnlockLayerRom,

    SYNOPSIS
	AROS_LHA(struct Layer *, l,   A5),

    LOCATION
	struct GfxBase *, GfxBase, 73, Graphics)

    FUNCTION
        Unlocks a previously locked layer for access by other applications
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
        LockLayerRom(), hyperlayers.library/LockLayer()
	hyperlayers.library/UnLockLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
	#include "aros/m68k/asm.h"

	.text
	.balign 2
	.globl	AROS_SLIB_ENTRY(UnlockLayerRom,Graphics,73)

AROS_SLIB_ENTRY(UnlockLayerRom,Graphics,73):
    move.l  %a6,%sp@-
    move.l  SysBase,%a6
    lea.l   %a5@(ly_Lock),%a0
    jsr     %a6@(ReleaseSemaphore)
    move.l  %sp@+,%a6
    rts
