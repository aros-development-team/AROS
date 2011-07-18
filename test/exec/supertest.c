/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>

extern UWORD a(void);
extern UWORD b(void);

#ifdef __GNUC__
#ifdef __mc68000
#define HAVE_ASM_CODE
asm(
"	.text\n"
"	.balign 2\n"
"	.globl	a\n"
"a:\n"
"	movew	%sr,%d0\n"
"	rts\n"
"	.globl	b\n"
"b:\n"
"	movew	%sp@,%d0\n"
"	rte\n"
);
#endif
#ifdef __i386__
#define HAVE_ASM_CODE
__asm__(
"	.globl	a\n"
"a:\n"
"	pushf\n"
"	popl	%eax\n"
"	ret\n"
"	.globl	b\n"
"b:\n"
"	movl	%esp,%eax\n"
"	iret\n"
);
#endif
#ifdef __x86_64__
#define HAVE_ASM_CODE
__asm__(
"	.globl	a\n"
"a:\n"
"	pushf\n"
"	popq	%rax\n"
"	ret\n"
"	.globl	b\n"
"b:\n"
"	movq	%rsp, %rax\n"
"	iret\n"
);
#endif
#endif

#ifndef HAVE_ASM_CODE
#define a(A) 0
#undef Supervisor
#define Supervisor(A) 0
#endif

int main(void)
{
    APTR ssp;
    UWORD ar;

    Printf("GetCC(): %04x\n",GetCC());
    Printf("SetSR(): %04lx\n",SetSR(0,0));
    ssp = SuperState();
    ar = a();
    UserState(ssp);
    Printf("Supervisor flags: %04x\n", ar);
    Printf("User flags: %04x\n", a());
    Printf("User stack pointer: 0x%p\n", &ssp);
    Printf("Supervisor stack pointer: 0x%p\n", Supervisor((ULONG_FUNC)b));

    return 0;
}

