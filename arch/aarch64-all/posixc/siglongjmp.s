/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX.1-2008 function siglongjmp() for AArch64
    Lang: english
*/

    #include "aros/aarch64/asm.h"

    .text
    _ALIGNMENT
    .globl  AROS_CDEFNAME(siglongjmp)
    _FUNCTION(AROS_CDEFNAME(siglongjmp))

AROS_CDEFNAME(siglongjmp):
    /* x0 = pointer to jmp_buf, w1 = return value */
    /* Make sure return value is not 0 */
    cmp     w1, #0
    csinc   w1, w1, wzr, ne

    /* Restore callee-saved FP registers d8-d15 */
    ldp     d8, d9, [x0, #112]
    ldp     d10, d11, [x0, #128]
    ldp     d12, d13, [x0, #144]
    ldp     d14, d15, [x0, #160]

    /* Restore callee-saved registers x19-x30 and sp */
    ldp     x19, x20, [x0, #8]
    ldp     x21, x22, [x0, #24]
    ldp     x23, x24, [x0, #40]
    ldp     x25, x26, [x0, #56]
    ldp     x27, x28, [x0, #72]
    ldp     x29, x30, [x0, #88]
    /* Return via the retaddr slot (patched by vfork machinery) */
    ldr     x30, [x0, #retaddr]
    ldr     x2, [x0, #104]
    mov     sp, x2

    mov     w0, w1
    ret
