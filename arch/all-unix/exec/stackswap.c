/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Change the stack of a task.
    Lang: english
*/
#include <setjmp.h>

static jmp_buf env;

#ifdef __linux__
#   define SP(env)      ((APTR)(env[0].__sp))
#   define FP(env)      ((APTR)(env[0].__bp))
#   define PC(env)      ((APTR)(env[0].__pc))
#endif

#ifdef TEST
#   define PROTO_EXEC_H
#   define EXEC_TASKS_H

#   include <exec/types.h>

#   define AROS_LH1(ret,name,a,lt,lb,o,ln) \
	ret name (a, lt lb)
#   define AROS_LHA(t,n,r)  t n

#   define Disable() /* eps */
#   define Enable() /* eps */

struct Task
{
    APTR	tc_SPReg;
    APTR	tc_SPLower;
    APTR	tc_SPUpper;
};

struct StackSwapStruct
{
    APTR  stk_Lower;   /* Lowest byte of stack */
    ULONG stk_Upper;   /* Upper end of stack (size + Lowest) */
    APTR  stk_Pointer; /* Stack pointer at switch point */
};

struct Task dummy;

#   define FindTask(arg)    (!(arg) ? &dummy : NULL)

struct ExecBase * SysBase = 0x0BAD0BAD;

#endif

/******************************************************************************

    NAME */
#include <exec/tasks.h>
#include <proto/exec.h>

	AROS_LH1(void, StackSwap,

/*  SYNOPSIS */
	AROS_LHA(struct StackSwapStruct *, sss, A0),

/*  LOCATION */
	struct ExecBase *, SysBase, 122, Exec)

/*  FUNCTION
	Change the stack of a task.

    INPUTS
	sss - The description of the new stack

    RESULT
	There will be a new stack.

    NOTES
	Calling this routine the first time will change sss and
	calling it a second time, the changes will be undone.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	This is a symmetrical routine. If you call it twice, then
	everything will be as it was before.

    HISTORY

******************************************************************************/
{
    /* Protect env */
    Disable ();

    if (!(setjmp (env)) )
    {
	struct Task * this;

	IPTR * newSP;
	IPTR * oldSP;
	APTR   ptr1;
	APTR   ptr2;
	int    t;

	newSP = (IPTR *)(sss->stk_Pointer);
	oldSP = (IPTR *)SP(env);

	/*
	    Init the new stack. After that it should look like this:

	    3	SysBase
	    2	sss
	    1	Return Address
	    0	FP
	*/

	t = ((IPTR)FP(env) - (IPTR)oldSP)/sizeof(APTR) + 4;

#ifdef TEST
    printf ("%d elements\n", t);
#endif

	newSP -= t; /* Make room for 20 elements */

	/*
	    Move the contents of the old stack on the new stack.
	    We will copy local variables, the frame pointer, the
	    return address and the parameters sss and SysBase.
	*/
	for ( ; oldSP != FP(env); ) /* Copy local vars */
	    *newSP++ = *oldSP++;

	FP(env) = newSP; /* Set new frame pointer */

	for (t=0; t<4; t++) /* Copy FP, return address, sss and SysBase */
	    *newSP++ = *oldSP++;

	newSP = (IPTR *)(sss->stk_Pointer) - 20;

	sss->stk_Pointer = oldSP; /* Save modified old SP */

	SP(env) = newSP; /* Set new SP */

	this = FindTask (NULL);

	/* Exchange stack bounds between task and sss */
	ptr1 = this->tc_SPLower;
	ptr2 = sss->stk_Lower;
	this->tc_SPLower = ptr2;
	sss->stk_Lower	 = ptr1;

	ptr1 = this->tc_SPUpper;
	ptr2 = (IPTR *)sss->stk_Upper;
	this->tc_SPUpper = ptr2;
	sss->stk_Upper	 = (ULONG)ptr1;


	longjmp (env, 1); /* Switch stack */
    }

    /* This is on the new stack */
    Enable ();
} /* StackSwap */

#if 0
	#include "machine.i"

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	.type	AROS_SLIB_ENTRY(StackSwap,Exec),@function

	/* The stack looks like this:

	    8 SysBase
	    4 sss
	    0 return address
	*/

#define sss	    4
#define SysBase     8

AROS_SLIB_ENTRY(StackSwap,Exec):
	/* Read parameter sss */
	movl sss(%esp),%edx

	/* copy new SP into ecx */
	movl stk_Pointer(%edx),%ecx

	/* Pop return address and sss from the current stack and copy them
	    onto the one specified in sss */
	popl %eax		    /* pop Return address */
	movl %eax,-12(%ecx)         /* Push return address on new stack */
	popl %eax		    /* pop sss */
	movl %eax,-8(%ecx)          /* Push sss on new stack */

	/* Copy SysBase from the current stack onto the one in sss */
	movl (%esp),%eax
	movl %eax,-4(%ecx)          /* Push SysBase on new stack */

	/* Calc new start of stack in sss */
	addl $-12,%ecx

	/* Call Disable() (SysBase is still on the stack) */
	leal Disable(%eax),%eax
	call *%eax
	popl %eax		    /* Remove SysBase from current stack */

	movl %esp,stk_Pointer(%edx) /* Save current SP in sss */
	movl %ecx,%esp		    /* Load the new stack */

	movl ThisTask(%eax),%ecx
	leal tc_SPLower(%ecx),%ecx  /* ecx = &SysBase->ThisTask->tc_SPLower */

	push %ebx		    /* Save register */

	/* Swap ThisTask->tc_SPLower and sss->stk_Lower */
	movl stk_Lower(%edx),%eax
	movl (%ecx),%ebx
	movl %eax,(%ecx)
	movl %ebx,stk_Lower(%edx)

	/* Swap tc_SPUpper and sss->stk_Upper, too */
	movl stk_Upper(%edx),%eax
	movl 4(%ecx),%ebx
	movl %eax,4(%ecx)
	movl %ebx,stk_Upper(%edx)

	popl %ebx		    /* Restore register */

	/* Call Enable() */
	movl SysBase(%esp),%eax
	pushl %eax		    /* push SysBase on new stack */
	leal Enable(%eax),%eax      /* call enable */
	call *%eax
	addl $4,%esp		    /* Clean stack */

	/* Note that at this time, the new stack from sss contains the
	   same values as the previous stack */
	ret
#endif

#ifdef TEST

ULONG teststack[4096];

void printstack (void)
{
    int addr;

    printf ("addr=%p\n", &addr);
}

int main (int argc, char ** argv)
{
    struct StackSwapStruct sss;

    sss.stk_Lower = teststack;
    sss.stk_Pointer = &teststack[sizeof(teststack) / sizeof(teststack[0])];
    sss.stk_Upper = (ULONG)sss.stk_Pointer;

    printf ("teststack = %p\n", sss.stk_Pointer);

    printstack ();

    StackSwap (&sss, SysBase);
    printstack ();

    StackSwap (&sss, SysBase);
    printstack ();

    StackSwap (&sss, SysBase);
    printstack ();

    StackSwap (&sss, SysBase);
    printstack ();

    return 0;
}

#endif
