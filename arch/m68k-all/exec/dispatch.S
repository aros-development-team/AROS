/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dispatch()
    Lang: english

    These routines are the core of the m68k scheduling system
*/

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Dispatch,Exec,10)
        .globl  __Dispatch_this

        .func AROS_SLIB_ENTRY(Dispatch,Exec,10)
AROS_SLIB_ENTRY(Dispatch,Exec,10):
        lea.l   %sp@(-4*(8+8)),%sp      /* %sp = regs_t */
        move.l  %sp,%sp@-               /* Push regs_t * onto stack */
__Dispatch_this:
        jsr     cpu_Dispatch            /* Call cpu_Dispatch */
        addq.l  #4,%sp
        movem.l %sp@+,%d0-%d7/%a0-%a5   /* Load task state */
        move.l  %sp@(4),%a6
        move.l  %a6,%usp                /* Restore user stack pointer */
        move.l  %sp@,%a6                /* Restore A6 */
        addq.l  #8,%sp
        rte                             /* SR, PC, etc are on the stack */  
        .endfunc
