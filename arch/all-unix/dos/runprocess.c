/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS utility function RunProcess
    Lang: english
*/
#include <aros/asmcall.h> /* LONG_FUNC */
#ifndef TEST
#   include <dos/dos.h>
#   include <dos/dosextens.h>
#   include <proto/exec.h>
#else
#   include <exec/types.h>
#   include <exec/tasks.h>

struct Process;

struct DosLibrary
{
    struct ExecBase * dl_SysBase;
};

extern void StackSwap (struct StackSwapStruct *, struct ExecBase *);

#define StackSwap(s)            StackSwap (s, SysBase)
#define AROS_SLIB_ENTRY(a,b)    a

#endif /* TEST */

#define SysBase     (DOSBase->dl_SysBase)
#include <stdio.h>
/******************************************************************************

    NAME */
	LONG AROS_SLIB_ENTRY(RunProcess,Dos) (

/*  SYNOPSIS */
	struct Process	       * proc,
	struct StackSwapStruct * sss,
	STRPTR			 argptr,
	ULONG			 argsize,
	LONG_FUNC		 entry,
	struct DosLibrary      * DOSBase)

/*  FUNCTION
	Sets the stack as specified and calls the given routine with the
	given arguments.

    INPUTS
	proc - Process context
	sss - New stack
	argptr - Pointer to argument string
	argsize - Size of the arguments
	entry - The entry point of the function
	DOSBase - Pointer to dos.library

    RESULT
	The return value of (*entry)();

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
#if 0
	/* Move upper bounds of the new stack into eax */
	movl stk_Upper(%ebx),%eax

	/*
	    Push arguments for entry onto the stack of the new process.
	    This new stack looks like this when the new process is called:

		sss
		SysBase
		argsize
		argptr
	*/
	movl argptr(%esp),%edx
	movl %edx,-16(%eax)
	movl argsize(%esp),%edx
	movl %edx,-12(%eax)

	/* Push sss onto the new stack */
	movl %ebx,-4(%eax)

	/* Get SysBase */
	movl DOSBase(%esp),%edx
	movl dl_SysBase(%edx),%edx

	/* Push SysBase on the new stack */
	movl %edx,-8(%eax)
#endif
    APTR  * oldSP;
    APTR  * sp;
    ULONG * retptr;
    ULONG   ret;

    retptr = &ret;

    sp = (APTR *)(sss->stk_Upper);

    oldSP = (APTR *)&DOSBase;

    fprintf(stderr, "CIAOAOAOAOAO\n");
    /* Copy stack + locals + regs + everything */
    while (oldSP != (APTR *)&ret)
	*--sp = *oldSP --;

#if 0
	/* Store switch point in sss */
	addl $-16,%eax
	movl %eax,stk_Pointer(%ebx)
#endif

    sss->stk_Pointer = sp;

#if 0
	/* Push SysBase and sss on our stack */
	pushl %edx /* SysBase */
	pushl %ebx /* sss */

	/* Switch stacks */
	leal StackSwap(%edx),%edx
	call *%edx

	/* Clean new stack from call to StackSwap */
	addl $8,%esp
#endif
printf("In runprocess! %lx , %lx\n", (IPTR)entry, (IPTR)*entry);
    StackSwap (sss);

    /* Call the function with the new stack. */
/*    *retptr = (*entry)(argptr, argsize, DOSBase->dl_SysBase);
*/
#ifdef __mc68000__
	*retptr = AROS_UFC3R(ULONG, entry,
		    AROS_UFCA(STRPTR, argptr  ,A0),
		    AROS_UFCA(ULONG,  argsize ,D0),
		    AROS_UFCA(struct ExecBase *, SysBase, A6),
		    &proc->pr_ReturnAddr
		  );
#else
	*retptr = AROS_UFC3(ULONG, entry,
		    AROS_UFCA(STRPTR, argptr  ,A0),
		    AROS_UFCA(ULONG,  argsize ,D0),
		    AROS_UFCA(struct ExecBase *, SysBase, A6)
		  );
#endif

#if 0
	/* Call the specified routine */
	call *%edi

	/* Clean (new) stack partially, leaving SysBase behind */
	addl $8,%esp

	/* Store the result of the routine in esi */
	movl %eax,%esi

	/* Swap the upper two values on the new stack */
	popl %edx /* SysBase */
	popl %ebx /* sss */
	pushl %edx /* SysBase */
	pushl %ebx /* sss */

	/* Switch stacks back */
	leal StackSwap(%edx),%edx
	call *%edx

	/* Clean old stack */
	addl $8,%esp

	/* Put the result in eax where our caller expects it */
	movl %esi,%eax

	/* Restore registers */
	popl %ebp
	popl %ebx
	popl %esi
	popl %edi

	/* Done */
	ret
#endif

    StackSwap (sss);

    return ret;
} /* RunProcess */

#ifdef TEST

#undef SysBase

#include <stdio.h>

ULONG teststack[4096];

int DemoProc (const char * argstr, int argsize, struct ExecBase * SysBase)
{
    printf ("arg=\"%s\" (len=%d)\n", argstr, argsize);

    return argsize;
} /* DemoProc */

int main (int argc, char ** argv)
{
    int ret, len;
    char * argstr;
    struct StackSwapStruct sss;
    struct DosLibrary	   DosBase;

    sss.stk_Lower = teststack;
    sss.stk_Pointer = &teststack[sizeof(teststack) / sizeof(teststack[0])];
    sss.stk_Upper = (ULONG)sss.stk_Pointer;

    DosBase.dl_SysBase = (struct ExecBase *)0x0bad0bad;

    printf ("Stack=%p\n", &ret);

    argstr = "Hello world.";

    len = strlen (argstr);

    ret = RunProcess (NULL
	, &sss
	, argstr
	, len
	, (LONG_FUNC)DemoProc
	, &DosBase
    );

    printf ("Stack=%p\n", &ret);

    printf ("RunProcess=%d\n", ret);

    if (len == ret)
	printf ("Test ok.\n");
    else
	printf ("Test failed.\n");

    return 0;
} /* main */

#endif /* TEST */
