/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Switch()
    Lang: english

    These routines are the core of the m68k scheduling system
*/

#include "aros/m68k/asm.h"

        .text
        .balign 4
        .globl  AROS_SLIB_ENTRY(Switch,Exec,9)

        .func AROS_SLIB_ENTRY(Switch,Exec,9)
AROS_SLIB_ENTRY(Switch,Exec,9):
        movem.l %d0-%d7/%a0-%a7,%sp@-
        move.l  %usp,%a0
        move.l  %a0,%sp@(4*15)          /* Fix up %a7 */
        move.l  %sp,%sp@-               /* Push regs_t * onto stack */
        jsr     cpu_Switch              /* Call cpu_Switch */
        bra     __Dispatch_this         /* Skip ahead */
        .endfunc
