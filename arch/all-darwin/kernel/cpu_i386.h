/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This thing is defined in sys/_types.h which conflicts with AROS include.
 * FIXME: such hacks are not good, perhaps we should supply
 * "-I$(GENINCDIR) -nostdinc -idirafter /usr/include" to AROS gcc when
 * building exec and kernel
 * P.S. exec should really not depend on these things.
 */
#define __darwin_sigset_t __uint32_t

#include <machine/_types.h>
#include <sys/ucontext.h>

typedef ucontext_t regs_t;

#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	__sighandler_t *

#define SC_DISABLE(sc)   (sc->sc_mask = sig_int_mask)
#define SC_ENABLE(sc)    (sigemptyset(&sc->sc_mask))

/* work around silly renaming of struct members in OS X 10.5 */
#if __DARWIN_UNIX03 && defined(_STRUCT_X86_EXCEPTION_STATE32)

#define R0(context)     ((context)->uc_mcontext->__ss.__eax)
#define R1(context)     ((context)->uc_mcontext->__ss.__ebx)
#define R2(context)     ((context)->uc_mcontext->__ss.__ecx)
#define R3(context)     ((context)->uc_mcontext->__ss.__edx)
#define R4(context)     ((context)->uc_mcontext->__ss.__edi)
#define R5(context)     ((context)->uc_mcontext->__ss.__esi)
#define R6(context)     ((context)->uc_mcontext->__ss.__eflags)

#define FP(context)     ((context)->uc_mcontext->__ss.__ebp)
#define PC(context)     ((context)->uc_mcontext->__ss.__eip))
#define SP(context)     ((context)->uc_mcontext->__ss.__esp))

#define FPSTATE(context) ((context)->uc_mcontext->__fs)

#else

#define R0(context)     ((context)->uc_mcontext->ss.eax)
#define R0(context)     ((context)->uc_mcontext->ss.eax)
#define R1(context)     ((context)->uc_mcontext->ss.ebx)
#define R2(context)     ((context)->uc_mcontext->ss.ecx)
#define R3(context)     ((context)->uc_mcontext->ss.edx)
#define R4(context)     ((context)->uc_mcontext->ss.edi)
#define R5(context)     ((context)->uc_mcontext->ss.esi)
#define R6(context)     ((context)->uc_mcontext->ss.eflags)

#define FP(context)     ((context)->uc_mcontext->ss.ebp)
#define PC(context)     ((context)->uc_mcontext->ss.eip))
#define SP(context)     ((context)->uc_mcontext->ss.esp))

#define FPSTATE(context) ((context)->uc_mcontext->fs)

#endif 

/*
 * We can't have an #ifdef based on FreeBSD here because this structure
 * is (wrongly) accessed from rom/exec.
 */
struct AROSCPUContext
{
    ULONG regs[9];	/* eax, ebx, ecx, edx, edi, esi, isp, fp, pc */
    int	errno_backup;
  _STRUCT_X86_FLOAT_STATE64 fpstate;
	int eflags;
};

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, struct sigcontext *sc) \
	{						     \
	    sighandler( sig, (regs_t*)sc);             \
	}

#define SAVE_CPU(cc,sc)                                              \
    do {                                                            \
	(cc)->regs[0] = R0(sc);                                     \
	(cc)->regs[1] = R1(sc);                                     \
	(cc)->regs[2] = R2(sc);                                     \
	(cc)->regs[3] = R3(sc);                                     \
	(cc)->regs[4] = R4(sc);                                     \
	(cc)->regs[5] = R5(sc);                                     \
	(cc)->regs[6] = R6(sc);                                     \
        (cc)->regs[7] = FP(sc);                                     \
        (cc)->regs[8] = PC(sc);                                     \
    } while (0)

#define RESTORE_CPU(cc,sc)                                          \
    do {                                                            \
	R0(sc) = (cc)->regs[0];                                     \
	R1(sc) = (cc)->regs[1];                                     \
	R2(sc) = (cc)->regs[2];                                     \
	R3(sc) = (cc)->regs[3];                                     \
	R4(sc) = (cc)->regs[4];                                     \
	R5(sc) = (cc)->regs[5];                                     \
	R6(sc) = (cc)->regs[6];                                     \
        FP(sc) = (cc)->regs[7];                                     \
        PC(sc) = (cc)->regs[8];                                     \
    } while (0)

#ifndef NO_FPU

#       define SAVE_FPU(cc,sc)                                              \
            do {                                                            \
				(cc)->fpstate = FPSTATE(sc);								\
            } while (0)

#       define RESTORE_FPU(cc,sc)                                           \
            do {                                                            \
			  FPSTATE(sc) = (cc)->fpstate;									\
            } while (0)

#       define HAS_FPU(sc)      1

#       define PREPARE_INITIAL_CONTEXT(cc)                                  \
            do {                                                            \
            } while (0)

#else
    /* NO FPU VERSION */

#   define SAVE_FPU(cc,sc)                                          \
        do {                                                        \
        } while (0)

#   define RESTORE_FPU(cc,sc)                                       \
        do {                                                        \
        } while (0)

#   define HAS_FPU(sc)      0

#   define PREPARE_INITIAL_CONTEXT(cc)                              \
        do {                                                        \
        } while (0)

#endif

#define PREPARE_INITIAL_FRAME(ctx, sp, startpc)     \
    do {                                            \
        ctx->regs[7] = 0;                           \
        ctx->regs[8] = (startpc);                   \
    } while (0)

#define SAVEREGS(cc, sc)                                            \
    do {                                                            \
        if (HAS_FPU(sc))                                            \
            SAVE_FPU((cc),sc);                                      \
        SAVE_CPU((cc),sc);                                          \
    } while (0)

#define RESTOREREGS(cc, sc)                                         \
    do {                                                            \
	RESTORE_CPU((cc),sc);                                       \
        if (HAS_FPU(sc))                                            \
            RESTORE_FPU((cc),sc);                                   \
	} while (0)

#define PRINT_SC(sc) \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n" \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#define PRINT_CPU_CONTEXT(ctx) \
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
