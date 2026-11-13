/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX.1-2008 function sigsetjmp() for AArch64
    Lang: english
*/

    #include "aros/aarch64/asm.h"

    .text
    _ALIGNMENT
    .globl  AROS_CDEFNAME(sigsetjmp)
    _FUNCTION(AROS_CDEFNAME(sigsetjmp))

AROS_CDEFNAME(sigsetjmp):
    /* x0 = pointer to jmp_buf, w1 = savesigs (unused in AROS) */
    /* Save return address (LR) into retaddr slot */
    str     x30, [x0, #retaddr]

    /* Save callee-saved registers x19-x30 and sp into regs[] */
    stp     x19, x20, [x0, #8]
    stp     x21, x22, [x0, #24]
    stp     x23, x24, [x0, #40]
    stp     x25, x26, [x0, #56]
    stp     x27, x28, [x0, #72]
    stp     x29, x30, [x0, #88]
    mov     x1, sp
    str     x1, [x0, #104]

    /* Save callee-saved FP registers d8-d15 */
    stp     d8, d9, [x0, #112]
    stp     d10, d11, [x0, #128]
    stp     d12, d13, [x0, #144]
    stp     d14, d15, [x0, #160]

    /* Return 0 */
    mov     w0, #0
    ret
