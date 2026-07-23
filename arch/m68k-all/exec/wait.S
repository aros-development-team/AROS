/* 68000 fast path for Exec/Wait(). */

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Wait,Exec,53)

        .func AROS_SLIB_ENTRY(Wait,Exec,53)
AROS_SLIB_ENTRY(Wait,Exec,53):
        movem.l %d2-%d3/%a2-%a3/%a5,%sp@-
        move.l  %d0,%d2
        move.l  ThisTask(%a6),%a2
        moveq   #0,%d3
        move.b  IDNestCnt(%a6),%d3
        move.l  %d2,tc_SigWait(%a2)
        move.b  TDNestCnt(%a6),tc_TDNestCnt(%a2)

        /*
         * Keep the immediate-signal case cheap.  The original nesting value
         * remains in D3 across a context switch.
         */
        move.w  #0x4000,0xdff09a
        move.b  %d3,%d1
        addq.b  #1,%d1
        move.b  %d1,IDNestCnt(%a6)

1:
        move.l  tc_SigRecvd(%a2),%d0
        and.l   %d2,%d0
        bne.s   3f

        /* Reopen interrupts before the trap and context save. */
        move.b  %d3,IDNestCnt(%a6)
        tst.b   %d3
        bpl.s   2f
        move.w  #0xc000,0xdff09a
2:

        lea.l   m68k_VoluntarySwitch,%a5
        jsr     Supervisor(%a6)

        move.b  tc_TDNestCnt(%a2),TDNestCnt(%a6)
        bra.s   1b

3:
        not.l   %d2
        and.l   %d2,tc_SigRecvd(%a2)

        /* Restore the caller's interrupt nesting state. */
        move.b  %d3,IDNestCnt(%a6)
        tst.b   %d3
        bpl.s   4f
        move.w  #0xc000,0xdff09a
4:
        movem.l %sp@+,%d2-%d3/%a2-%a3/%a5
        rts
        .endfunc
