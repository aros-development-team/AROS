/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __AROS_EXEC_LIBRARY__

#include <signal.h>

#define EXCEPTIONS_COUNT 17

typedef struct sigcontext regs_t;
#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	void *	

#define SC_DISABLE(sc)   (sc->sc_mask = KernelBase->kb_PlatformData->sig_int_mask)
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
#define R6(sc)           (sc->sc_eflags) 

#define GLOBAL_SIGNAL_INIT(sighandler)						   \
	static void sighandler ## _gate (int sig, int code, struct sigcontext *sc) \
	{						     			   \
	    sighandler( sig, (regs_t*)sc);       				   \
	}

#define SAVE_CPU(ctx, sc)	 \
	(ctx->regs[0] = R0(sc)), \
	(ctx->regs[1] = R1(sc)), \
	(ctx->regs[2] = R2(sc)), \
	(ctx->regs[3] = R3(sc)), \
	(ctx->regs[4] = R4(sc)), \
	(ctx->regs[5] = R5(sc)), \
	(ctx->regs[6] = R6(sc))

#define RESTORE_CPU(ctx, sc)	 \
	((R0(sc) = ctx->regs[0]), \
	(R1(sc) = ctx->regs[1]), \
	(R2(sc) = ctx->regs[2]), \
	(R3(sc) = ctx->regs[3]), \
	(R4(sc) = ctx->regs[4]), \
	(R5(sc) = ctx->regs[5]), \
	(R6(sc) = ctx->regs[6]))

#define HAS_FPU		0
#define SAVE_FPU(task,sc)	/* nop */
#define RESTORE_FPU(task,sc)	/* nop */

#define PREPARE_INITIAL_FRAME(sp,pc) 	/* nop */

#define SAVEREGS(ctx, sc)     \
	((ctx)->pc = PC(sc)), \
	((ctx)->fp = FP(sc)), \
	/* SAVE_FPU((ctx), sc), */ \
	SAVE_CPU((ctx), sc)

#define RESTOREREGS(ctx, sc)    \
	RESTORE_CPU((ctx), sc), \
	/* RESTORE_FPU((ctx), sc), */ \
	(FP(sc) = (ctx)->fp), \
	(PC(sc) = (ctx)->pc)

#define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

struct AROSCPUContext
{
    ULONG regs[7];	/* eax, ebx, ecx, edx, edi, esi, isp */
    ULONG pc,fp;	/* store these on the stack to avoid sighandlers */
    int	errno_backup;
};

#define GET_PC(ctx) ((APTR)ctx->pc)
#define SET_PC(ctx, val) ctx->pc = (ULONG)val

#define PREPARE_INITIAL_FRAME(ctx, sp,startpc) \
	ctx->pc = (ULONG)startpc; \
	ctx->fp = 0

#define PRINT_CPUCONTEXT(ctx) \
	printf ("    FP=%08lx  PC=%08lx\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , ctx->fp, ctx->pc, \
	    , ctx->regs[0] \
	    , ctx->regs[1] \
	    , ctx->regs[2] \
	    , ctx->regs[3] \
	    , ctx->regs[4] \
	    , ctx->regs[5] \
	    , ctx->regs[6] \
	)
