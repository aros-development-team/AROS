/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX function vfork() for AArch64
    Lang: english
*/

    #include "aros/aarch64/asm.h"

    .text
    _ALIGNMENT
    .globl  AROS_CDEFNAME(vfork)
    _FUNCTION(AROS_CDEFNAME(vfork))

AROS_CDEFNAME(vfork):
    /*
     * Allocate jmp_buf ((1 + _JMPLEN) * 8 = 256 bytes, see
     * aros/stdc/setjmp.h) plus a 16-byte slot for the original SP and
     * LR. They must survive the call to setjmp, and x8-x15 are
     * caller-saved (a linklib stub for setjmp may clobber them), so they
     * are stashed on the stack, not in scratch registers.
     */
    sub     sp, sp, #272
    add     x1, sp, #272        /* original SP */
    stp     x1, x30, [sp, #256] /* stash original SP and LR above the jmp_buf */

    mov     x0, sp              /* x0 = jmp_buf pointer */
    bl      setjmp              /* Fill jmp_buf with current state */

    /* Patch the jmp_buf: return to our caller, on our caller's stack */
    ldp     x2, x3, [sp, #256]  /* x2 = original SP, x3 = original LR */
    str     x3, [sp, #retaddr]  /* retaddr = caller's return address */
    str     x2, [sp, #104]      /* regs[12] = original sp */

    mov     x0, sp              /* x0 = jmp_buf for __vfork */
    bl      __vfork             /* __vfork won't return */
