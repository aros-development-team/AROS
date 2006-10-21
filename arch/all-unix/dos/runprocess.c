/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/

#include <aros/asmcall.h>	/* LONG_FUNC */

#include <dos/dosextens.h>
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include <ucontext.h>
#include <stdarg.h>
#include <boost/preprocessor/repetition/enum.hpp>

#define ARGS(z, n, text) text[n] 
#define MAXARGS 16

static void SwapTaskStackLimits(APTR *lower, APTR *upper)
{
    struct Task* this = FindTask(NULL);
    APTR tmp;

    tmp              = this->tc_SPLower;
    this->tc_SPLower = *lower;
    *lower           = tmp;
	
    tmp              = this->tc_SPUpper;
    this->tc_SPUpper = *upper;
    *upper           = tmp;
}

static void CallEntry(
	ULONG*			Return_Value,
	APTR		        pReturn_Addr,
	struct StackSwapStruct* sss,
	STRPTR			argptr,
	ULONG			argsize,
	LONG_FUNC		entry)
{
#ifndef AROS_UFC3R
#error You need to write the AROS_UFC3R macro for your CPU
#endif

    *Return_Value
	= AROS_UFC3R(ULONG, entry,
		AROS_UFCA(STRPTR, argptr, A0),
		AROS_UFCA(ULONG, argsize, D0),
		AROS_UFCA(struct ExecBase *, SysBase, A6),
		pReturn_Addr,
		(sss->stk_Upper - (ULONG)sss->stk_Lower) /* used by m68k-linux arch, needed?? */
	      );
}

static void trampoline(void (*func)(), IPTR args[])
{
    /* this was called from CallWithStack which also called Disable */
    Enable();

    /*
    {
	int test;
    
	D(bug("swapped context: test = 0x%x\n", &test));
    }
    */
    
    func(BOOST_PP_ENUM(MAXARGS, ARGS, args));

    /* this was called from CallWithStack which will enable again */        
    Disable();
}

BOOL CallWithStack
(
    void (*func)(), void* stack, size_t size, int argc, IPTR args[]
)
{
    ucontext_t ucx, ucx_return;
    
    if (argc > MAXARGS || getcontext(&ucx) == -1)
        return FALSE;
    
    ucx.uc_stack.ss_sp    = stack;
    ucx.uc_stack.ss_size  = size;
    ucx.uc_stack.ss_flags = SS_ONSTACK;
    
    ucx.uc_link = &ucx_return;
    
    makecontext(&ucx, (void (*)()) trampoline, 2, func, args);
    
    APTR SPLower = stack, SPUpper = stack + size;
    #if AROS_STACK_DEBUG
    {
	UBYTE* startfill     = SPLower;
	const UBYTE* endfill = SPUpper;
	    
	while (startfill < endfill)
	{
	    *startfill++ = 0xE1;
	}
   }
   #endif
    /*
       we enable again in trampoline, after we have swapped
       the new stack borders into the task structure
    */
    Disable();
    SwapTaskStackLimits(&SPLower, &SPUpper);

    BOOL success = swapcontext(&ucx_return, &ucx) != -1;

    SwapTaskStackLimits(&SPLower, &SPUpper);
    Enable();

    return success; 
}

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
    ULONG ret;
    APTR  oldReturnAddr = proc->pr_ReturnAddr; /* might be changed by CallEntry */
    IPTR  args[]     	=
                        {
			    (IPTR) &ret,
			    (IPTR) &proc->pr_ReturnAddr,
			    (IPTR) sss,
			    (IPTR) argptr,
			    argsize == -1 ? strlen(argptr) : argsize, /* Compute argsize automatically */
			    (IPTR) entry
			};

    /*
    {
	int test;
	D(bug("before callwithstack; org context: test = 0x%x\n", &test));
    }
    */
    
    /* Call the function with the new stack */
    CallWithStack(
	CallEntry,
	(void *) sss->stk_Lower,
	(size_t) sss->stk_Upper - (size_t) sss->stk_Lower,
	6,
	args);
	
    /*
    {
	int test;
	D(bug("after  callwithstack; org context: test = 0x%x\n", &test));
    }
    */

    proc->pr_ReturnAddr = oldReturnAddr;

    return ret;
}
