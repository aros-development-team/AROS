/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the stack of a task.
    Lang: english
*/
#include <jmpdefs.h>
#include <aros/config.h>

static jmp_buf env;

#define DEBUG 1

#ifdef TEST
/*******************************************************
********************************************************
***						     ***
***   Compile this with -DTEST to get a standalone   ***
***   program to check if it works on your system.   ***
***						     ***
********************************************************
*******************************************************/

/* I use this command line to compile it:

    gcc -DTEST -I/usr/include -I ../../bin/linux-i386/AROS/include/ \
	    -g stackswap.c -o t

    -DTEST
	    Enable the special test code
    -I/usr/include
	    Use the system's includes over AROS' ones (AROS comes with some
	    include files with the same names as the system ones).
    -I../../bin/linux-i386/AROS/include/
	    Specify the correct directory for your system (ie. replace
	    "linux-i386" by your OS and CPU)
    -g
	    Include debug infos in the final executable
    stackswap.c
	    Compile this code
    -o t
	    Write the executable to "t".

*/

#   define PROTO_EXEC_H
#   define EXEC_TASKS_H

#   include <exec/types.h>

#   define AROS_LH1(ret,name,a,lt,lb,o,ln) \
	ret name (a, lt lb)
#   define AROS_LHA(t,n,r)  t n

#   define Disable() /* eps */
#   define Enable() /* eps */

#   include <stdio.h>

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

struct ExecBase * SysBase = (struct ExecBase *)0x0BAD0BAD;

#endif

#ifndef SP
#error SP(env) undefined
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
    AROS_LIBFUNC_INIT

    /* Protect env */
    Disable ();

    if (!(setjmp (env)) )
    {
	struct Task * this;

	IPTR * newSP;
	IPTR * oldSP;
	APTR   ptr1;
	APTR   ptr2;
	int    t, i;

	newSP = (IPTR *)(sss->stk_Pointer);
	oldSP = (IPTR *)SP(env);

	/*
	    Init the new stack. After that it should look like this:

	    3	SysBase
	    2	sss
	    1	Return Address
	    0	FP
	*/

	t = ((IPTR)FP(env) - (IPTR)oldSP)/sizeof(APTR) + NUM_LONGS;

#if DEBUG
#ifdef TEST
    printf ("%d elements\n", t);
#endif
#endif

	newSP -= t; /* Make room for t elements */

    #if AROS_STACK_DEBUG && 0
        
	{
	    UBYTE *startfill = sss->stk_Lower;
	    UBYTE *endfill = ((UBYTE *)newSP) - 16;
	    
	    while(startfill <= endfill)
	    {
	        *startfill++ = 0xE1;
	    }
	}
	
    #endif
 
	/*
	    Move the contents of the old stack on the new stack.
	    We will copy local variables, the frame pointer, the
	    return address and the parameters sss and SysBase.
	*/
	for ( ; oldSP != FP(env); ) /* Copy local vars */
	    *newSP++ = *oldSP++;

	FP(env) = newSP; /* Set new frame pointer */

	for (i=0; i<NUM_LONGS; i++) /* Copy NUM_LONGS */
	    *newSP++ = *oldSP++;

	newSP = (IPTR *)(sss->stk_Pointer) - t;

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

    AROS_LIBFUNC_EXIT
} /* StackSwap */

#ifndef NO_MAIN
#ifdef TEST

ULONG teststack[4096];

int count = 0;
APTR stacks[5];

void checkstack (void)
{
    int addr;

#if DEBUG
    printf ("addr=%p\n", &addr);
#endif

    stacks[count++] = &addr;
}

int main (int argc, char ** argv)
{
    struct StackSwapStruct sss;

    sss.stk_Lower = teststack;
    sss.stk_Pointer = &teststack[sizeof(teststack) / sizeof(teststack[0])];
    sss.stk_Upper = (ULONG)sss.stk_Pointer;

#if DEBUG
    printf ("teststack = %p\n", sss.stk_Pointer);
#endif

    checkstack ();

    StackSwap (&sss, SysBase);
    checkstack ();

    StackSwap (&sss, SysBase);
    checkstack ();

    StackSwap (&sss, SysBase);
    checkstack ();

    StackSwap (&sss, SysBase);
    checkstack ();

    if (stacks[0] != stacks[2] || stacks[1] != stacks[3] || stacks[2] != stacks[4])
	printf ("Test failed!\n");
    else
	printf ("Test ok.\n");

    return 0;
}

#endif /* TEST */
#endif /* !NO_MAIN */
