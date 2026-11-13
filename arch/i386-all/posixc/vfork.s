/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX function vfork(), i386 version
    Lang: english
*/
    #include "aros/i386/asm.h"

    .text
    _ALIGNMENT
    .globl AROS_CDEFNAME(vfork)
    _FUNCTION(AROS_CDEFNAME(vfork))
    .set    bufsize, 8*4
    .set    stack,   7*4

AROS_CDEFNAME(vfork):
    lea     (-bufsize)(%esp), %esp /* _JMPLEN + 1 longs on the stack
		                      it's our temporary jmp_buf */
    push    %esp
    call    setjmp                 /* fill jmp_buf on the stack with
	                              current register values */
    pop     %esp
    mov     bufsize(%esp), %eax    /* set return address in jmp_buf */
    mov     %eax, retaddr(%esp)    /* to this function call return 
	                              address */
    lea     bufsize(%esp), %eax    /* set stack value in jmp_buf */
                                   /* to stack value from before */
    mov     %eax, stack(%esp)      /* this function call */

    mov     %esp, %eax
    push    %ebp
    mov     %esp, %ebp
    push    %eax
    call    __vfork                /* __vfork call won't return */
