/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <signal.h>
#include <errno.h>
#include "etask.h"

/* Put a value of type SP_TYPE on the stack or get it off the stack. */
#define _PUSH(sp,val)       (*--sp = (SP_TYPE)(val))
#define _POP(sp)            (*sp++)

typedef struct sigcontext sigcontext_t;
#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	__sighandler_t *

#define SP_TYPE		long
#define CPU_NUMREGS	0

#define SC_DISABLE(sc)   (sc->sc_mask = sig_int_mask)
#define SC_ENABLE(sc)    (sigemptyset(&sc->sc_mask))

#define SP(sc)       (sc->sc_esp)
#define FP(sc)       (sc->sc_ebp)
#define PC(sc)       (sc->sc_eip)

#define R0(sc)           (sc->sc_eax)
#define R1(sc)           (sc->sc_ebx)
#define R2(sc)           (sc->sc_ecx)
#define R3(sc)           (sc->sc_edx)
#define R4(sc)           (sc->sc_edi)
#define R5(sc)           (sc->sc_esi)
#define R6(sc)           (sc->sc_isp)

struct AROS_cpu_context
{
    ULONG regs[7];	/* eax, ebx, ecx, edx, edi, esi, isp */
    ULONG pc,fp;	/* store these on the stack to avoid sighandlers */
    int	errno_backup;
};

#define SIZEOF_ALL_REGISTERS	(sizeof(struct AROS_cpu_context))
#define GetCpuContext(task)	((struct AROS_cpu_context *)\
				(GetIntETask(task)->iet_Context))
#define GetSP(task)		((SP_TYPE*)(task->tc_SPReg))

#define GLOBAL_SIGNAL_INIT \
	static void sighandler (int sig, sigcontext_t * sc); \
							     \
	static void SIGHANDLER (int sig, int code, struct sigcontext *sc) \
	{						     \
	    sighandler( sig, (sigcontext_t*)sc);       \
	}

#define SAVE_CPU(task, sc) \
	(GetCpuContext(task)->regs[0] = R0(sc)), \
	(GetCpuContext(task)->regs[1] = R1(sc)), \
	(GetCpuContext(task)->regs[2] = R2(sc)), \
	(GetCpuContext(task)->regs[3] = R3(sc)), \
	(GetCpuContext(task)->regs[4] = R4(sc)), \
	(GetCpuContext(task)->regs[5] = R5(sc)), \
	(GetCpuContext(task)->regs[6] = R6(sc))

#define RESTORE_CPU(task,sc) \
	((R0(sc) = GetCpuContext(task)->regs[0]), \
	(R1(sc) = GetCpuContext(task)->regs[1]), \
	(R2(sc) = GetCpuContext(task)->regs[2]), \
	(R3(sc) = GetCpuContext(task)->regs[3]), \
	(R4(sc) = GetCpuContext(task)->regs[4]), \
	(R5(sc) = GetCpuContext(task)->regs[5]), \
	(R6(sc) = GetCpuContext(task)->regs[6]))

#define SAVE_ERRNO(task) \
	(GetCpuContext(task)->errno_backup = errno)
	
#define RESTORE_ERRNO(task) \
	(errno = GetCpuContext(task)->errno_backup)

#define HAS_FPU		0
#define SAVE_FPU(task,sc)	/* nop */
#define RESTORE_FPU(task,sc)	/* nop */

#define PREPARE_INITIAL_FRAME(sp,pc) 	/* nop */

#define PREPARE_INITIAL_CONTEXT(task,startpc) \
	( GetCpuContext(task)->pc = (ULONG)startpc, \
	  GetCpuContext(task)->fp = 0 )

#define SAVEREGS(task,sc) \
	((GetSP(task) = (long *)SP(sc)), \
	(GetCpuContext(task)->pc = PC(sc)), \
	(GetCpuContext(task)->fp = FP(sc)), \
	/* SAVE_FPU(task, sc), */ \
	SAVE_CPU(task, sc), \
	SAVE_ERRNO(task))

#define RESTOREREGS(task,sc) \
	(RESTORE_ERRNO(task), \
	RESTORE_CPU(task,sc), \
	/* RESTORE_FPU(task, sc), */ \
	(FP(sc) = GetCpuContext(task)->fp), \
	(PC(sc) = GetCpuContext(task)->pc)), \
	(SP(sc) = (long)GetSP(task))

#define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#define PRINT_CPUCONTEXT(task) \
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

#endif /* _SIGCORE_H */
