#ifndef _SIGCORE_H
#define _SIGCORE_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros to handle i386 Linux signals
    Lang: english
*/

#include <aros/i386/cpucontext.h>

/* Allocate legacy 80087 frame together with SSE frame */
#define USE_LEGACY_8087

/* Allocate 112 bytes for 8087 frame. It contains host_specific portion (status and magic). */
#define SIZEOF_8087_FRAME 112

#ifdef __AROS_EXEC_LIBRARY__

struct sigcontext;
typedef struct sigcontext regs_t;

#else

#define USE_SA_SIGINFO 0

#if USE_SA_SIGINFO
#define SIGCORE_NEED_SA_SIGINFO 1
#include <ucontext.h>
#endif

#include <signal.h>

/* Include generated fragment */
#include <sigcore.h>

/* name and type of the signal handler */
#define SIGHANDLER	linux_sighandler
#define SIGHANDLER_T	SignalHandler

/*
    This macro contains some magic necessary to make it work.
    The problem is that Linux offers no official way to obtain the
    signals' context. Linux stores the signals' context on the
    process' stack. It looks like this:
    Attention: As of version 2.2 of the Linux kernel there is
               not enough room on the stack anymore to save
               any registers on it. So everything has to go into
               the context structure. Previously PC and FP used 
               to be saved on the stack but now this would over-
               write some return address.
               The stack you see below is the stack of the last 
               active task within AROS. The linux kernel puts
               all kinds of other junk on it.

		    |			       |
		    +--------------------------+
		    | last entry before signal |
		    +--------------------------+
		    |	   signal context      |
		    +--------------------------+
		    |	   signal number       |
		    +--------------------------+
		    |	   return address      | 
		    +--------------------------+
		    |			       |

    so the address of the signal context is &sig+1.
*/

#if USE_SA_SIGINFO
#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, siginfo_t *blub, struct ucontext *u) 	\
	{ 										\
	    sighandler(sig, (regs_t *)&u->uc_mcontext); 				\
	}

#else

#define GLOBAL_SIGNAL_INIT(sighandler)			\
	static void sighandler ## _gate (int sig)       \
	{						\
	    sighandler (sig, (regs_t *)(&sig+1));       \
	}
#endif

/*
    Macros to access the stack pointer, frame pointer and program
    counter. The FP is the base address for accesses to arguments
    and local variables of a function and PC is the current address
    in the program code.
*/

#define SP(sc)           ((sc)->esp)
#define FP(sc)           ((sc)->ebp)
#define PC(sc)           ((sc)->eip)

/*
    Macros to enable or disable all signals after the signal handler
    has returned and the normal execution commences.

    WARNING!!! If you change #define USE_SA_SIGINFO to 1, this will
    stop working! In this case Linux will use ucontext->uc_sigmask
    to store original signal mask. See x86-64 and PPC implementation
    of these macros for examples.
*/
#ifdef HOST_OS_android
/* In Android's Bionic sigset_t is simply unsigned long */
#define SC_DISABLE(sc)   ((sc)->oldmask = KernelBase->kb_PlatformData->sig_int_mask)
#else
#define SC_DISABLE(sc)   ((sc)->oldmask = KernelBase->kb_PlatformData->sig_int_mask.__val[0])
#endif
#define SC_ENABLE(sc)    ((sc)->oldmask = 0L)

/*
    The names of the general purpose registers which are to be saved.
    Use R and a number as name, no matter what the real name is.
    General purpose registers (GPRs) are registers which can be
    modified by the task (ie. data and address registers) and which are
    not saved by the CPU when an interrupt happens.
*/
#define R0(sc)           ((sc)->eax)
#define R1(sc)           ((sc)->ebx)
#define R2(sc)           ((sc)->ecx)
#define R3(sc)           ((sc)->edx)
#define R4(sc)           ((sc)->edi)
#define R5(sc)           ((sc)->esi)
#define R6(sc)           ((sc)->eflags)

/* Save and restore the CPU GPRs in the CPU context */
#define SAVE_CPU(cc, sc)	\
    cc.eax    = R0(sc);		\
    cc.ebx    = R1(sc);		\
    cc.ecx    = R2(sc);		\
    cc.edx    = R3(sc);		\
    cc.edi    = R4(sc);		\
    cc.esi    = R5(sc);		\
    cc.eflags = R6(sc);		\
    cc.ebp    = FP(sc);		\
    cc.eip    = PC(sc);		\
    cc.esp    = SP(sc);

#define RESTORE_CPU(cc, sc) \
    R0(sc) = cc.eax;        \
    R1(sc) = cc.ebx;        \
    R2(sc) = cc.ecx;        \
    R3(sc) = cc.edx;        \
    R4(sc) = cc.edi;        \
    R5(sc) = cc.esi;        \
    R6(sc) = cc.eflags;     \
    FP(sc) = cc.ebp;        \
    PC(sc) = cc.eip;        \
    SP(sc) = cc.esp;


/*
    It's not possible to save the FPU under linux because linux
    uses the tasks stack to save the signal context. The signal
    context conatins the SP *before* the sigcontext was pushed on
    this stack, so it looks like this:

		    |			       |
		    +--------------------------+
		    | last entry before signal |
		    +--------------------------+
		    |	    empty space        | <--- SP
		    +--------------------------+
		    |	   signal context      |
		    +--------------------------+
		    |			       |


    As you can see, SP points to the empty space. Now this empty space
    is not very big. It's big enough that one can save the CPU
    registers but not big enough for the FPU. *sigh*.

    Attention: The above WAS TRUE for 2.0.x kernels but now the stack layout
               looks different. See above!

    Update: We store the registers in our own structure now
*/

/*
 * This macro saves all registers. Use this macro when you
 * want to leave the current task's context.
 * If will also save FPU and SSE areas (if possible) and
 * set appropriate context flags to indicate this.
 */
#define SAVEREGS(cc, sc)                                       					\
    SAVE_CPU((cc)->regs, sc);									\
    if ((cc)->regs.FPData && sc->fpstate)							\
    {												\
	(cc)->regs.Flags |= ECF_FPU;								\
	CopyMemQuick(sc->fpstate, (cc)->regs.FPData, SIZEOF_8087_FRAME);			\
	if ((cc)->regs.FXData && (sc->fpstate->magic != 0xFFFF))				\
    	{											\
    	    (cc)->regs.Flags |= ECF_FPX;							\
    	    CopyMemQuick(sc->fpstate->_fxsr_env, (cc)->regs.FXData, sizeof(struct FPXContext));	\
    	}											\
    }

/*
 * This macro does the opposite to SAVEREGS(). It restores all registers.
 * After that, you can enter the new task's context.
 * FPU and SSE areas will be restored only if they were present in the
 * saved context. This should be indicated by context's flags.
 */
#define RESTOREREGS(cc, sc)									\
    RESTORE_CPU((cc)->regs, sc);								\
    if (sc->fpstate)										\
    {												\
    	if ((cc)->regs.Flags & ECF_FPU)								\
    	    CopyMemQuick((cc)->regs.FPData, sc->fpstate, SIZEOF_8087_FRAME);			\
    	if ((cc)->regs.Flags & ECF_FPX)								\
    	    CopyMemQuick((cc)->regs.FXData, sc->fpstate->_fxsr_env, sizeof(struct FPXContext));	\
    }

/* This macro prints the current signals' context */
#define PRINT_SC(sc) \
    bug ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
	 "    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
	 "    R4=%08lx  R5=%08lx\n" \
	 , SP(sc), FP(sc), PC(sc) \
	 , R0(sc), R1(sc), R2(sc), R3(sc) \
	 , R4(sc), R5(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 17

/* Use this structure to save/restore registers */
struct AROSCPUContext
{
    struct ExceptionContext regs; /* Public portion		  */
    int errno_backup;		  /* Host-specific stuff, private */
};

#endif /* _SIGCORE_H */
