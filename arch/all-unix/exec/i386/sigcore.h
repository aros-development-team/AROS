/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <signal.h>
#include "etask.h"

/* Put a value of type SP_TYPE on the stack or get it off the stack. */
#define _PUSH(sp,val)       (*--sp = (SP_TYPE)(val))
#define _POP(sp)            (*sp++)

#ifdef __linux__
#   include <asm/sigcontext.h>
#   include <linux/version.h>
    /* sigcontext_t is the type of the signals' context. Linux offers no way
	to get this context in a legal way, so I have to use tricks. */
#   ifdef i386
#     if LINUX_VERSION_CODE > 131102
	typedef struct sigcontext sigcontext_t;
#     else
	typedef struct sigcontext_struct sigcontext_t;
#     endif
#   else
	typedef struct sigcontext sigcontext_t;
#   endif

    /* name and type of the signal handler */
#   define SIGHANDLER	    linux_sighandler
#   define SIGHANDLER_T     SignalHandler

    /*
	This macro contains some magic necessary to make it work.
	The problem is that Linux offers no official way to obtain the
	signals' context. Linux stores the signals' context on the
	process' stack. It looks like this:

		    |			       |
		    +--------------------------+
		    | last entry before signal |
		    +--------------------------+
		    |	    empty space        | <--- SP
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
#   define GLOBAL_SIGNAL_INIT \
	static void sighandler (int sig, sigcontext_t * sc);    \
								\
	static void SIGHANDLER (int sig)                        \
	{							\
	    sighandler (sig, (sigcontext_t *)(&sig+1));         \
	}

    /* Type of the values which can be stored on the stack. A variable
	which is to be used as a stack pointer must be declared as
	"SP_TYPE *". */
#   define SP_TYPE	    long

    /* How many general purpose registers are to be saved on the stack
	when a task switch happens */
#   define CPU_NUMREGS		    0

    /* Use this structure to save/restore registers if the stack is too
	small */
    struct AROS_cpu_context
    {
	ULONG regs[6]; /* eax, ebx, ecx, edx, edi, esi */
	struct _fpstate fpu; /* FPU registers */
    };

#   define SIZEOF_ALL_REGISTERS     (sizeof (struct AROS_cpu_context))
#   define GetCpuContext(task)      ((struct AROS_cpu_context *)\
				    (GetIntETask(task)->iet_Context))
#   define GetSP(task)              ((SP_TYPE*)(task->tc_SPReg))

    /*
	Macros to access the stack pointer, frame pointer and program
	counter. The FP is the base address for accesses to arguments
	and local variables of a function and PC is the current address
	in the program code.
    */
#   define SP(sc)           (sc->esp)
#   define FP(sc)           (sc->ebp)
#   define PC(sc)           (sc->eip)

    /*
	Macros to enable or disable all signals after the signal handler
	has returned and the normal execution commences.
    */
#   define SC_DISABLE(sc)   (sc->oldmask = ~0L)
#   define SC_ENABLE(sc)    (sc->oldmask = 0L)

    /*
	The names of the general purpose registers which are to be saved.
	Use R and a number as name, no matter what the real name is.
	General purpose registers (GPRs) are registers which can be
	modified by the task (ie. data and address registers) and which are
	not saved by the CPU when an interrupt happens.
    */
#   define R0(sc)           (sc->eax)
#   define R1(sc)           (sc->ebx)
#   define R2(sc)           (sc->ecx)
#   define R3(sc)           (sc->edx)
#   define R4(sc)           (sc->edi)
#   define R5(sc)           (sc->esi)

    /*
	Save and restore the CPU GPRs in the CPU context
    */
#   define SAVE_CPU(task,sc) \
	(GetCpuContext(task)->regs[0] = R0(sc)), \
	(GetCpuContext(task)->regs[1] = R1(sc)), \
	(GetCpuContext(task)->regs[2] = R2(sc)), \
	(GetCpuContext(task)->regs[3] = R3(sc)), \
	(GetCpuContext(task)->regs[4] = R4(sc)), \
	(GetCpuContext(task)->regs[5] = R5(sc))

#   define RESTORE_CPU(task,sc) \
	(R0(sc) = GetCpuContext(task)->regs[0]), \
	(R1(sc) = GetCpuContext(task)->regs[1]), \
	(R2(sc) = GetCpuContext(task)->regs[2]), \
	(R3(sc) = GetCpuContext(task)->regs[3]), \
	(R4(sc) = GetCpuContext(task)->regs[4]), \
	(R5(sc) = GetCpuContext(task)->regs[5])

    /*
	It's not possible to do save the FPU under linux because linux
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

	Update: We store the registers now in out own structure
    */
/* #   define NO_FPU */

    /*
	Size of the FPU stackframe in stack units (one stack unit is
	sizeof(SP_TYPE) bytes).
    */
#   ifndef NO_FPU
#	define FPU_FRAMESIZE	(sizeof (struct _fpstate) / sizeof (SP_TYPE))
#   else
#	define FPU_FRAMESIZE	0
#   endif

    /*
	This macro return 1 if a FPU is available.
    */
#   ifndef NO_FPU
#	define HAS_FPU(sc)      (sc->fpstate)
#   else
#	define HAS_FPU(sc)      0
#   endif

    /*
	Save and restore the FPU on/from the stack.
    */
#   define SAVE_FPU(task,sc) \
	HAS_FPU(sc) && \
	    ((GetCpuContext(task)->fpu = *sc->fpstate), 1)

#   define RESTORE_FPU(task,sc) \
	HAS_FPU(sc) && \
	    ((*sc->fpstate = GetCpuContext(task)->fpu), 1)

    /*
	Prepare the stack. This macro is used on the stack before a new
	task is run for the first time. To create such a macro, you must
	know how the system uses the stack. On Linux/i386, every stack
	frame looks like this:

						 high adresses
		    |	       ...	     |
		    +------------------------+
		    |	    arguments	     |
		    +------------------------+
		    |	  return address     |
		    +------------------------+
		    |	old frame pointer    |
		    +------------------------+
		    |	 local variables     |
		    +------------------------+
		    |	 saved registers     |
		    +------------------------+
		    |	       ...	     |
						low addresses
						stack grows from high to
						low addresses.

	The first routine gets no arguments, but if you want to pass
	some to it, then you must push them on the stack before you
	call this macro. Note that the arguments must be pushed in
	reverse order, ie. if you want to call a function like this:

	    func (a,b,c);

	then you must prepare the stack like this:

	    _PUSH(sp,c);
	    _PUSH(sp,b);
	    _PUSH(sp,a);
	    PREPARE_INITIAL_FRAME(sp,func);

	This is because the arguments are fetched relative to the FP
	(ie. FP[0] is the old frame pointer, FP[1] is the return
	address, FP[2] is the first argument, FP[3] is the second
	and so on).

    */
#   define PREPARE_INITIAL_FRAME(sp,pc) \
	(_PUSH(sp,pc), \
	_PUSH(sp,0)) /* Frame pointer */

    /*
	Prepare the cpu context
    */
#   define PREPARE_INITIAL_CONTEXT(task,startpc)    /* nop */

    /*
	This macro is similar to PREPARE_INITIAL_FRAME() but also saves
	all general purpose registers. Use this macro when you want to
	leave the current tasks' context to save the registers. Note that
	the argument "sp" of the macro is just the name of the stack
	pointer. The macro will load it from the sigcontext "sc". You
	must store the value of "sp" after the macro and hand it to
	RESTOREREGS() below to restore this context.
    */
#   define SAVEREGS(task,sc) \
	((GetSP(task) = (long *)SP(sc)), \
	_PUSH(GetSP(task),PC(sc)), \
	_PUSH(GetSP(task),FP(sc)), \
	SAVE_FPU(task,sc), \
	SAVE_CPU(task,sc))

    /*
	This macro does the opposite to SAVEREGS(). It restores all
	general purpose registers. After that, you can enter the new
	tasks' context. Both "sp" and "sc" must be initialized.
	The macro will save the new SP into the sigcontext "sc".
    */
#   define RESTOREREGS(task,sc) \
	(RESTORE_CPU(task,sc), \
	RESTORE_FPU(task,sc), \
	(FP(sc) = _POP(GetSP(task))), \
	(PC(sc) = _POP(GetSP(task))), \
	(SP(sc) = (long)(GetSP(task))))

    /* This macro prints the current signals' context */
#   define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc) \
	)

    /* This macro prints the current stack (after SAVEREGS()) */
#   define PRINT_CPUCONTEXT(task) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx\n" \
	    , (ULONG)(GetSP(task))) \
	    , GetSP(task)[-1] \
	    , GetSP(task)[-2] \
	    , GetCpuContext(task)->regs[0] \
	    , GetCpuContext(task)->regs[1] \
	    , GetCpuContext(task)->regs[2] \
	    , GetCpuContext(task)->regs[3] \
	    , GetCpuContext(task)->regs[4] \
	    , GetCpuContext(task)->regs[5] \
	)

#endif /* __linux__ */

#ifdef __FreeBSD__
typedef struct sigcontext sigcontext_t;
#   define SIGHANDLER	   bsd_sighandler
#   define SIGHANDLER_T    __sighandler_t *

#   define SP_TYPE	long
#   define CPU_NUMREGS	0

#   define SC_DISABLE(sc)   (sc->sc_mask = ~0L)
#   define SC_ENABLE(sc)    (sc->sc_mask = 0L)

#   define SP(sc)       (sc->sc_esp)
#   define FP(sc)       (sc->sc_ebp)
#   define PC(sc)       (sc->sc_eip)

#   define R0(sc)           (sc->sc_eax)
#   define R1(sc)           (sc->sc_ebx)
#   define R2(sc)           (sc->sc_ecx)
#   define R3(sc)           (sc->sc_edx)
#   define R4(sc)           (sc->sc_edi)
#   define R5(sc)           (sc->sc_esi)
#   define R6(sc)           (sc->sc_isp)

    struct AROS_cpu_context
    {
	ULONG regs[7]; /* eax, ebx, ecx, edx, edi, esi, isp */
	ULONG pc,fp;	/* store these on the stack to avoid sighandlers */
    };

#   define SIZEOF_ALL_REGISTERS	(sizeof(struct AROS_cpu_context))
#   define GetCpuContext(task)	((struct AROS_cpu_context *)\
				(GetIntETask(task)->iet_Context))
#   define GetSP(task)		((SP_TYPE*)(task->tc_SPReg))

#   define GLOBAL_SIGNAL_INIT \
	static void sighandler (int sig, sigcontext_t * sc); \
							     \
	static void SIGHANDLER (int sig, int code, struct sigcontext *sc) \
	{						     \
	    sighandler( sig, (sigcontext_t*)sc);       \
	}

#   define SAVE_CPU(task, sc) \
	(GetCpuContext(task)->regs[0] = R0(sc)), \
	(GetCpuContext(task)->regs[1] = R1(sc)), \
	(GetCpuContext(task)->regs[2] = R2(sc)), \
	(GetCpuContext(task)->regs[3] = R3(sc)), \
	(GetCpuContext(task)->regs[4] = R4(sc)), \
	(GetCpuContext(task)->regs[5] = R5(sc)), \
	(GetCpuContext(task)->regs[6] = R6(sc))

#   define RESTORE_CPU(task,sc) \
	((R0(sc) = GetCpuContext(task)->regs[0]), \
	(R1(sc) = GetCpuContext(task)->regs[1]), \
	(R2(sc) = GetCpuContext(task)->regs[2]), \
	(R3(sc) = GetCpuContext(task)->regs[3]), \
	(R4(sc) = GetCpuContext(task)->regs[4]), \
	(R5(sc) = GetCpuContext(task)->regs[5]), \
	(R6(sc) = GetCpuContext(task)->regs[6]))

#   define HAS_FPU		0
#   define SAVE_FPU(task,sc)	/* nop */
#   define RESTORE_FPU(task,sc) /* nop */

#   define PREPARE_INITIAL_FRAME(sp,pc) 	/* nop */

#   define PREPARE_INITIAL_CONTEXT(task,startpc) \
	( GetCpuContext(task)->pc = (ULONG)startpc, \
	  GetCpuContext(task)->fp = 0 )

#   define SAVEREGS(task,sc) \
	((GetSP(task) = (long *)SP(sc)), \
	(GetCpuContext(task)->pc = PC(sc)), \
	(GetCpuContext(task)->fp = FP(sc)), \
	/* SAVE_FPU(task, sc), */ \
	SAVE_CPU(task, sc))

#   define RESTOREREGS(task,sc) \
	(RESTORE_CPU(task,sc), \
	/* RESTORE_FPU(task, sc), */ \
	(FP(sc) = GetCpuContext(task)->fp), \
	(PC(sc) = GetCpuContext(task)->pc)), \
	(SP(sc) = (long)GetSP(task))

#   define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#   define PRINT_CPUCONTEXT(task) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , (ULONG)(GetSP(task)) \
	    , GetCpuContext(task)->fp, GetCpuContext(task)->pc, \
	    , GetCpuContext(task)->regs[0] \
	    , GetCpuContext(task)->regs[1] \
	    , GetCpuContext(task)->regs[2] \
	    , GetCpuContext(task)->regs[3] \
	    , GetCpuContext(task)->regs[4] \
	    , GetCpuContext(task)->regs[5] \
	    , GetCpuContext(task)->regs[6] \
	)

#endif /* __FreeBSD__ */

#endif /* _SIGCORE_H */
