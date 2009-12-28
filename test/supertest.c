/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>

extern UWORD a(void);
extern UWORD b(void);

#ifdef __GNUC__
#ifdef __mc68000
asm("
	.globl	_a
_a:
	movew	sr,d0
	rts
	.globl	_b
_b:
	movew	sp@,d0
	rte
");
#else
#ifdef __i386__
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
#else
#define a(A) 0
#undef Supervisor
#define Supervisor(A) 0
#endif
#endif
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
    Printf("Supervisor stack pointer: %04lx\n",
        (IPTR)Supervisor((ULONG_FUNC)&b));

    return 0;
}

