#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <signal.h>

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
#   define CPU_NUMREGS	    6

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
	Save and restore the CPU GPRs on/from the stack.
    */
#   define SAVE_CPU(sp,sc) \
	_PUSH(sp,R0(sc)), \
	_PUSH(sp,R1(sc)), \
	_PUSH(sp,R2(sc)), \
	_PUSH(sp,R3(sc)), \
	_PUSH(sp,R4(sc)), \
	_PUSH(sp,R5(sc))

#   define RESTORE_CPU(sp,sc) \
	(R5(sc) = _POP(sp)), \
	(R4(sc) = _POP(sp)), \
	(R3(sc) = _POP(sp)), \
	(R2(sc) = _POP(sp)), \
	(R1(sc) = _POP(sp)), \
	(R0(sc) = _POP(sp))

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
    */
#   define NO_FPU

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
#   ifndef NO_FPU
#	define SAVE_FPU(sp,sc) \
	    (sp -= FPU_FRAMESIZE), \
	    HAS_FPU(sc) && \
		((*((struct _fpstate *)sp) = *(sc->fpstate)), 1)

#	define RESTORE_FPU(sp,sc) \
	    HAS_FPU(sc) && \
		((*(sc->fpstate) = *((struct _fpstate *)sp)), 1), \
	    (sp += FPU_FRAMESIZE)
#   else
#	define SAVE_FPU(sp,sc)          (sp -= 0)
#	define RESTORE_FPU(sp,sc)       (sp += 0)
#   endif

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
	_PUSH(sp,0), /* Frame pointer */ \
	(sp -= (FPU_FRAMESIZE+CPU_NUMREGS)))

    /*
	This macro is similar to PREPARE_INITIAL_FRAME() but also saves
	all general purpose registers. Use this macro when you want to
	leave the current tasks' context to save the registers. Note that
	the argument "sp" of the macro is just the name of the stack
	pointer. The macro will load it from the sigcontext "sc". You
	must store the value of "sp" after the macro and hand it to
	RESTOREREGS() below to restore this context.
    */
#   define SAVEREGS(sp,sc) \
	((sp = (long *)SP(sc)), \
	_PUSH(sp,PC(sc)), \
	_PUSH(sp,FP(sc)), \
	SAVE_FPU(sp,sc), \
	SAVE_CPU(sp,sc))

    /*
	This macro does the opposite to SAVEREGS(). It restores all
	general purpose registers. After that, you can enter the new
	tasks' context. Both "sp" and "sc" must be initialized.
	The macro will save the new SP into the sigcontext "sc".
    */
#   define RESTOREREGS(sp,sc) \
	(RESTORE_CPU(sp,sc), \
	RESTORE_FPU(sp,sc), \
	(FP(sc) = _POP(sp)), \
	(PC(sc) = _POP(sp)), \
	(SP(sc) = (long)sp))

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
#   define PRINT_STACK(sp) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx\n" \
	    , (ULONG)(sp+(FPU_FRAMESIZE+CPU_NUMREGS+2)) \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS] \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS+1] \
	    , sp[5], sp[4], sp[3], sp[2] \
	    , sp[1], sp[0] \
	)

#endif /* __linux__ */

#ifdef __FreeBSD__
typedef struct sigcontext sigcontext_t;
#   define SIGHANDLER	   bsd_sighandler
#   define SIGHANDLER_T    __sighandler_t *

#   define SP_TYPE	long
#   define CPU_NUMREGS	7

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

#   define GLOBAL_SIGNAL_INIT \
	static void sighandler (int sig, sigcontext_t * sc); \
							     \
	static void SIGHANDLER (int sig, int code, struct sigcontext *sc) \
	{						     \
	    sighandler( sig, (sigcontext_t*)sc);       \
	}

#   define PREPARE_INITIAL_FRAME(sp,pc) \
	sp -= 128, \
	_PUSH(sp,pc), \
	_PUSH(sp,sp), \
	sp -= CPU_NUMREGS

#   define SAVEREGS(sp,sc) \
	sp = (SP_TYPE *)SP(sc), \
	sp -= 128, \
	_PUSH(sp,PC(sc)), \
	_PUSH(sp,FP(sc)), \
	_PUSH(sp,R0(sc)), \
	_PUSH(sp,R1(sc)), \
	_PUSH(sp,R2(sc)), \
	_PUSH(sp,R3(sc)), \
	_PUSH(sp,R4(sc)), \
	_PUSH(sp,R5(sc)), \
	_PUSH(sp,R6(sc))

#   define RESTOREREGS(sp,sc) \
	R6(sc) = _POP(sp), \
	R5(sc) = _POP(sp), \
	R4(sc) = _POP(sp), \
	R3(sc) = _POP(sp), \
	R2(sc) = _POP(sp), \
	R1(sc) = _POP(sp), \
	R0(sc) = _POP(sp), \
	FP(sc) = _POP(sp), \
	PC(sc) = _POP(sp), \
	sp += 128, \
	SP(sc) = (int)sp

#   define NO_FPU

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
#   ifndef NO_FPU
#	define SAVE_FPU(sp,sc) \
	    (sp -= FPU_FRAMESIZE), \
	    HAS_FPU(sc) && \
		((*((struct _fpstate *)sp) = *(sc->fpstate)), 1)

#	define RESTORE_FPU(sp,sc) \
	    HAS_FPU(sc) && \
		((*(sc->fpstate) = *((struct _fpstate *)sp)), 1), \
	    (sp += FPU_FRAMESIZE)
#   else
#	define SAVE_FPU(sp,sc)          (sp -= 0)
#	define RESTORE_FPU(sp,sc)       (sp += 0)
#   endif


#   define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#   define PRINT_STACK(sp) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , (ULONG)(sp+(FPU_FRAMESIZE+CPU_NUMREGS+2)) \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS] \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS+1] \
	    , sp[6], sp[5], sp[4], sp[3] \
	    , sp[2], sp[1], sp[0] \
	)

#endif /* __FreeBSD__ */

#endif /* _SIGCORE_H */
