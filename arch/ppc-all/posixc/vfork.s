/*
    Copyright © 2008-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function vfork(), PowerPC version
    Lang: english
*/

#include "aros/ppc/asm.h"

    .text
    _ALIGNMENT
    .globl AROS_CDEFNAME(vfork)
    _FUNCTION(AROS_CDEFNAME(vfork))
    .set    bufsize, 60*4
    .set    ret_addr, 2*4
    .set    stack,   0*4

AROS_CDEFNAME(vfork):

    mtctr  1               /* save current stack pointer */
    stwu   1, -bufsize(1)  /* _JMPLEN + 2 longs on the stack
                              it's our temporary jmp_buf
                              aligned on 16 byte */
    
    mflr  12               /* save return address */
    
    addi  3, 1, 8          /* prepare argument for setjmp */
    lis   11, setjmp@ha    /* get address of setjmp */
    la    11, setjmp@l(11)
    mtlr  11
    blrl                   /* fill jmp_buf on the stack with
                              current register values */
    
    stw   12, ret_addr+8(1) /* set return address in jmp_buf */
    mfctr 12
    stw   12, stack+8(1)   /* set stack value in jmp_buf */
    
    addi  3, 1, 8          /* prepare argument for __vfork */
    lis   11, __vfork@ha   /* get address of __vfork */
    la    11, __vfork@l(11)
    mtctr 11
    bctr                   /* __vfork call won't return */
