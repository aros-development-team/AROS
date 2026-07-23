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

        /* Disable(), kept inline because this is the latency-critical path. */
        move.w  #0x4000,0xdff09a
        addq.b  #1,IDNestCnt(%a6)

1:
        move.l  tc_SigRecvd(%a2),%d3
        and.l   %d2,%d3
        bne.s   3f

        move.l  %d2,tc_SigWait(%a2)
        move.b  TDNestCnt(%a6),tc_TDNestCnt(%a2)
        move.b  #-1,TDNestCnt(%a6)
        move.b  #4,tc_State(%a2)       /* TS_WAIT */

        /* AddTail(&SysBase->TaskWait, &task->tc_Node). */
        lea.l   TaskWait(%a6),%a0
        move.l  lh_TailPred(%a0),%a1
        lea.l   ln_Pred(%a0),%a3       /* address of the tail sentinel */
        move.l  %a3,ln_Succ(%a2)
        move.l  %a1,ln_Pred(%a2)
        move.l  %a2,ln_Succ(%a1)
        move.l  %a2,lh_TailPred(%a0)

        lea.l   m68k_VoluntarySwitch,%a5
        jsr     Supervisor(%a6)

        move.b  tc_TDNestCnt(%a2),TDNestCnt(%a6)
        bra.s   1b

3:
        not.l   %d2
        and.l   %d2,tc_SigRecvd(%a2)

        /* Enable(). */
        subq.b  #1,IDNestCnt(%a6)
        bpl.s   4f
        move.w  #0xc000,0xdff09a
4:
        move.l  %d3,%d0
        movem.l %sp@+,%d2-%d3/%a2-%a3/%a5
        rts
        .endfunc
