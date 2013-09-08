/*
    Copyright © 2008-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function vfork()
    Lang: english
*/

    #include "aros/x86_64/asm.h"

    .text
    _ALIGNMENT
    .globl AROS_CDEFNAME(vfork)
    _FUNCTION(AROS_CDEFNAME(vfork))
    .set    bufsize, 16*8
    .set    stack,   15*8

AROS_CDEFNAME(vfork):
    lea     (-bufsize)(%rsp), %rsp /* _JMPLEN + 1 longs on the stack
		                   it's our temporary jmp_buf */
    mov     %rsp, %rdi
    call    setjmp              /* fill jmp_buf on the stack with
	                           current register values */
    mov     bufsize(%rsp), %rax    /* set return address in jmp_buf */
    mov     %rax, 0(%rdi)       /* to this function call return 
	                           address */
    lea     bufsize(%rsp), %rax    /* set stack value in jmp_buf */
    mov     %rax, 120(%rdi)     /* this function call */
    call    __vfork             /* __vfork call won't return */
