/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ExitIntr()
    Lang: english

    These routines are the core of the m68k scheduling system
*/

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(ExitIntr,Exec,6)

        .func AROS_SLIB_ENTRY(ExitIntr,Exec,6)
AROS_SLIB_ENTRY(ExitIntr,Exec,6):
        /* At this point, %d0-%d1/%a0-%a1/%a5-%a6 are on
         * the stack, followed by SR, PC, and an exception frame
         */

        /* If we are returning to supervisor mode, just do it.
         * %sp@(4*6) is the SR from the RTE.
         * We want to test bit 12 (0x2000) of the SR.
         * But BTST only tests a byte. Therefore, we
         * only need to test bit 5 (0x20) of the upper
         * byte of SR, which is at %sp@(4*6)
         */
        btst.b  #5,%sp@(4*6)
        bne     0f

        /*
         * Determine whether we need to call the Scheduler
         */
        jsr     core_ExitIntr
        tst.w   %d0
        beq     0f
        jmp     __Schedule_this
0:
        movem.l %sp@+,%d0-%d1/%a0-%a1/%a5-%a6
        rte
        .endfunc
