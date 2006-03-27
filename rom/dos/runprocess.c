/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/
#include <aros/asmcall.h>	/* LONG_FUNC */
#ifndef TEST
#   include <dos/dosextens.h>
#   include <proto/exec.h>
#   include <aros/debug.h>
#else
#   include <exec/types.h>
#   include <exec/tasks.h>
#   define D(x)	/* eps */

struct Process;

struct DosLibrary
{
    struct ExecBase * dl_SysBase;
};

extern void StackSwap (struct StackSwapStruct *, struct ExecBase *);

#define StackSwap(s)	StackSwap(s, SysBase)
#define AROS_SLIB_ENTRY(a,b)	a

#endif /* TEST */

#include <string.h>


/**************************************************************************

    NAME */
	LONG AROS_SLIB_ENTRY(RunProcess,Dos) (

/*  SYNOPSIS */
	struct Process		* proc,
	struct StackSwapStruct  * sss,
	STRPTR			  argptr,
	ULONG			  argsize,
	LONG_FUNC		  entry,
	struct DosLibrary 	* DOSBase)

/* FUNCTION
	Sets the stack as specified and calls the routine with the given
	arguments.

    INPUTS
	proc	- Process context
	sss	- New Stack
	argptr	- Pointer to argument string
	argsize	- Size of the argument string
	entry	- The entry point of the function
	DOSBase	- Pointer to dos.library structure

    RESULT
	The return value of (*entry)();

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

**************************************************************************/
{
    APTR *	oldSP;
    APTR *	sp;
    ULONG *	retptr;
    ULONG	ret;
    APTR 	oldReturnAddr;

    retptr = &ret;

    sp = (APTR *)(sss->stk_Upper);
    oldSP = (APTR *)&DOSBase;
    oldReturnAddr = proc->pr_ReturnAddr;

    /* Compute argsize automatically */
    if (argsize == -1)
    {
	argsize = strlen(argptr);
    }

    /* Copy stack + locals + regs + everything */
    while ( oldSP != (APTR *)&ret )
    {
	*--sp = *oldSP--;
    }

    sss->stk_Pointer = sp;

    D(bug("In RunProcess() entry=%lx, *entry=%lx\n", (IPTR)entry, (IPTR)*entry));
    StackSwap(sss);

    /* Call the function with the new stack */
    /*
	We have to set the pr_ReturnAddr pointer to the correct value
	before we call the entry() otherwise some startup code will
	not work.

	This can be done rather more easily on the m68k than elsewhere.
    */
#ifndef AROS_UFC3R
#error You need to write the AROS_UFC3R macro for your CPU
#endif

   /* The AROS_UFC3R() macro doesn't work on my system (gcc 2.95.1, Linux 2.3.50)
    * this is the workaround I'm currently using:
    */

//    *retptr = entry(argptr,argsize,SysBase);

    *retptr = AROS_UFC3R(ULONG, entry,
		AROS_UFCA(STRPTR, argptr, A0),
		AROS_UFCA(ULONG, argsize, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6),
		&proc->pr_ReturnAddr, (sss->stk_Upper - (ULONG)sss->stk_Lower)
	      );

    StackSwap(sss);

    proc->pr_ReturnAddr = oldReturnAddr;

    return ret;
}

#ifdef TEST

#include <stdio.h>

ULONG teststack[4096];

int DemoProc (const char * argstr, int argsize, struct ExecBase * SysBase)
{
    printf ("arg=\"%s\" (len=%d\n", argstr, argsize);

    return argsize;
} /* DemoProc */

int main (int argc, char ** argv)
{
    int ret, len;
    char * argstr;
    struct StackSwapStruct sss;
    struct DosLibrary DosBase;

    sss.stk_Lower = teststack;
    sss.stk_Upper = &teststack[sizeof(teststack) / sizeof(teststack[0])];
    sss.stk_Pointer = sss.stk_Upper;

    DosBase.dl_SysBase = (struct ExecBase *)0x0bad0bad;

    printf ("Stack=%p\n", &ret);

    argstr = "Hello world.";

    len = strlen (argstr);

    ret = RunProcess (NULL,
	&sss,
	argstr,
	len,
	(LONG_FUNC)DemoProc,
	&DOSBase
    );

    printf ("Stack=%p\n", &ret);
    printf ("RunProcess=%d\n",ret);

    if (len == ret)
    {
	printf("Test ok.\n");
    }
    else
    {
	printf("Test failed.\n");
    }

    return 0;
}

#endif /* TEST */
