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

	typedef struct sigcontext_struct sigcontext_t;
 
    /* name and type of the signal handler */
#   define SIGHANDLER	    linux_sighandler
#   if defined(__GLIBC__) && (__GLIBC__ >= 2)
#      define SIGHANDLER_T  __sighandler_t
#   else
#      define SIGHANDLER_T  SignalHandler
#   endif

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
	extern void SIGHANDLER (int sig, long sigcode, sigcontext_t * scp);  \
								\
	void call_inthandlers(int sig) {			\
	    struct IntVector *iv;				\
								\
	    iv = &SysBase->IntVects[sig2tab[sig]];		\
	    if (iv->iv_Code)					\
	    {							\
		AROS_UFC5(void,iv->iv_Code,			\
		    AROS_UFCA(ULONG, 0, D1),			\
		    AROS_UFCA(ULONG, 0, A0),			\
		    AROS_UFCA(APTR,iv->iv_Data,A1),		\
		    AROS_UFCA(APTR,iv->iv_Code,A5),		\
		    AROS_UFCA(struct ExecBase *,SysBase,A6)	\
		);						\
	    }							\
	}



    /* Type of the values which can be stored on the stack. A variable
	which is to be used as a stack pointer must be declared as
	"SP_TYPE *". */
#   define SP_TYPE	    long

    /* How many general purpose registers are to be saved on the stack
	when a task switch happens */
#   define CPU_NUMREGS	0

    /* Use this structure to save/restore registers if the stack is too
	small */
    struct AROS_cpu_context
    {
	sigcontext_t sc;	/* Signal context struct */
	ULONG regs[11];		/* normal regs not in sigcontext_t */
	ULONG fpregs[6*3];	/* FP regs not in sigcontext_t */
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

#   define SP(sc)           (sc->sc_usp) 
#   define PC(sc)           (sc->sc_pc)

    /*
	Macros to enable or disable all signals after the signal handler
	has returned and the normal execution commences.
    */

#   define SC_DISABLE(sc)   (sc->sc_mask = ~0L)
#   define SC_ENABLE(sc)    (sc->sc_mask = 0L)

#   define SAVE_CPU(task,sc) /**/

#   define RESTORE_CPU(task,sc) /**/

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

#   define PREPARE_INITIAL_FRAME(sp,pc) /* nop */

    /*
	Prepare the cpu context
    */
#   define PREPARE_INITIAL_CONTEXT(task,pc) \
	(GetCpuContext(task)->sc.sc_pc = (LONG)pc)
    


    /*
	This macro is similar to PREPARE_INITIAL_FRAME() but also saves
	all general purpose registers. Use this macro when you want to
	leave the current tasks' context to save the registers. Note that
	the argument "sp" of the macro is just the name of the stack
	pointer. The macro will load it from the sigcontext "sc". You
	must store the value of "sp" after the macro and hand it to
	RESTOREREGS() below to restore this context.
    */
    

#   define SAVEREGS(task,sc) /**/

    /*
	This macro does the opposite to SAVEREGS(). It restores all
	general purpose registers. After that, you can enter the new
	tasks' context. Both "sp" and "sc" must be initialized.
	The macro will save the new SP into the sigcontext "sc".
    */

#   define RESTOREREGS(task,sc) /**/

    /* This macro prints the current signals' context */
#   define PRINT_SC(sc) \
	printf ("SC: SP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx \n" \
	    , SP(sc), PC(sc) \
	    , "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , (long)R4(sc), (long)R5(sc) \
	)

    /* This macro prints the current stack (after SAVEREGS()) */
#   define PRINT_STACK(sp) \
	printf ("    SP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx\n" \
	    , (ULONG)(sp+(FPU_FRAMESIZE+CPU_NUMREGS+2)) \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS] \
	    , sp[FPU_FRAMESIZE+CPU_NUMREGS+1] \
	    , sp[5], sp[4], sp[3], sp[2] \
	    , sp[1], sp[0] \
	)

#endif /* __linux__ */

#endif /* _SIGCORE_H */
