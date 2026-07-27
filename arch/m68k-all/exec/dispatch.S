/* Restore an m68k task context directly from its saved user-stack frame. */

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Dispatch,Exec,10)
        .globl  __Dispatch_this

        .func AROS_SLIB_ENTRY(Dispatch,Exec,10)
AROS_SLIB_ENTRY(Dispatch,Exec,10):
__Dispatch_this:
        /* A6 may still contain the caller's library base here. */
        move.l  4,%a6
        ori.w   #0x0700,%sr

        /* Fast path for the ordinary head-of-ready-list dispatch. */
        lea.l   TaskReady(%a6),%a0
        move.l  %a0@,%a3
        move.l  %a3@,%d0
        beq.s   1f
        cmp.b   #TS_READY,tc_State(%a3)
        bne.s   1f
        move.b  tc_Flags(%a3),%d1
        and.b   #TF_DISPATCH_SPECIAL,%d1
        bne.s   1f
        move.w  AttnFlags(%a6),%d1
        and.w   #AFF_FPU,%d1
        bne.s   1f

        move.l  %d0,%a0@
        move.l  %d0,%a1
        move.l  %a0,ln_Pred(%a1)
        addq.l  #1,DispCount(%a6)
        move.l  %a3,ThisTask(%a6)
        move.w  Quantum(%a6),Elapsed(%a6)
        and.w   #SFF_QuantumClear,SysFlags(%a6)
        move.b  #TS_RUN,tc_State(%a3)
        move.b  tc_IDNestCnt(%a3),IDNestCnt(%a6)
        move.l  tc_SPReg(%a3),%a5
        bra.s   2f

1:
        jsr     m68k_DispatchFrame
        move.l  %d0,%a5

2:
        tst.b   IDNestCnt(%a6)
        bmi.s   3f
        move.w  #0x4000,0xdff09a
        bra.s   4f
3:
        move.w  #0xc000,0xdff09a
4:
        lea.l   %a5@(66),%a2
        move.l  %a2,%usp
        move.l  %a5@+,%sp@-
        move.w  %a5@+,%sp@-
        movem.l %a5@,%d0-%d7/%a0-%a6
        rte
        .endfunc
