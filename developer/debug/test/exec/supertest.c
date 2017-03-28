/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

extern IPTR a(void);
extern IPTR b(void);

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
"	iretq\n"
);
#endif
#endif

int __nocommandline = 1;

int main(void)
{
    APTR KernelBase = OpenResource("kernel.resource");
    struct Task *me = FindTask(NULL);
    APTR ssp;

    Printf("GetCC()             : %04lx\n",GetCC());
    Printf("SetSR()             : %08lx\n",SetSR(0,0));
    
    if (KernelBase)
    	Printf("KrnIsSuper()        : %d\n", KrnIsSuper());

    Printf("Task stack          : 0x%p - 0x%p\n", me->tc_SPLower, me->tc_SPUpper);
    Printf("Supervisor stack    : 0x%p - 0x%p\n", SysBase->SysStkLower, SysBase->SysStkUpper);

    ssp = SuperState();
    if (ssp)
    {
        bug("Entered SuperState...\n");

        int issuper = 0;
#ifdef HAVE_ASM_CODE
        IPTR ar = a();
#endif

        if (KernelBase)
    	    issuper = KrnIsSuper();

	bug("Leaving SuperState...\n");
    	UserState(ssp);

	if (KernelBase)
	    Printf("Supervisor mode test: %d\n", issuper);

	Printf("Saved stack         : 0x%p\n", ssp);
#ifdef HAVE_ASM_CODE
	Printf("Supervisor flags    : 0x%p\n", ar);
#endif
    }
    else
       	Printf("!!! SuperState() failed to enter supervisor mode (returned NULL) !!!\n");

#ifdef HAVE_ASM_CODE
    Printf("User flags          : 0x%p\n", a());
    Printf("Supervisor stack    : 0x%p\n", Supervisor(b));
#endif

    return 0;
}
