/* Restore an m68k task context directly from its saved user-stack frame. */

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Dispatch,Exec,10)
        .globl  __Dispatch_this

        .func AROS_SLIB_ENTRY(Dispatch,Exec,10)
AROS_SLIB_ENTRY(Dispatch,Exec,10):
__Dispatch_this:
        jsr     m68k_DispatchFrame
        move.l  %d0,%a5

        lea.l   %a5@(66),%a2
        move.l  %a2,%usp
        move.l  %a5@+,%sp@-
        move.w  %a5@+,%sp@-
        movem.l %a5@,%d0-%d7/%a0-%a6
        rte
        .endfunc
