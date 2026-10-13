/*
    Copyright © 2012-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Schedule()
    Lang: english

    These routines are the core of the m68k scheduling system
*/

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Schedule,Exec,7)
        .globl  __Schedule_this

        .func AROS_SLIB_ENTRY(Schedule,Exec,7)
AROS_SLIB_ENTRY(Schedule,Exec,7):
        movem.l %d0-%d1/%a0-%a1/%a5-%a6,%sp@-   /* Save D0-D1/A0-A1 */
__Schedule_this:
        /*
         * Decide with interrupts masked, as Kickstart's Schedule() does:
         * core_Schedule() clears SFF_AttnSwitch and looks at TaskReady, and
         * an interrupt in between could set the flag or queue a task. Switch
         * lowers the level again; otherwise the RTE restores it.
         */
        or.w    #0x0700,%sr
        jsr     core_Schedule
        tst.w   %d0                     /* Something to do? */
        movem.l %sp@+,%d0-%d1/%a0-%a1/%a5-%a6   /* Restore regs */
        beq     0f
        jmp     AROS_SLIB_ENTRY(Switch,Exec,9)
0:
        rte
        .endfunc
