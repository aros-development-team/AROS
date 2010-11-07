/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef __AROS_EXEC_LIBRARY__

#define _MC_FPOWNED_NONE 0x020000
#define _MC_FPFMT_NODEV  0x010000

/* FreeBSD v4 is an ancient history now */
#define __FreeBSD_version 500001

#else

#include <machine/psl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ucontext.h>
#include <signal.h>
#include "etask.h"

#define EXCEPTIONS_COUNT 17

typedef struct sigcontext regs_t;
#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	__sighandler_t *

#define SC_DISABLE(sc)   (sc->sc_mask = KernelBase->kb_PlatformData->sig_int_mask)
#define SC_ENABLE(sc)    (KernelIFace.sigemptyset(&sc->sc_mask))

#define SP(sc)           (sc->sc_esp)
#define FP(sc)           (sc->sc_ebp)
#define PC(sc)           (sc->sc_eip)

#define R0(sc)           (sc->sc_eax)
#define R1(sc)           (sc->sc_ebx)
#define R2(sc)           (sc->sc_ecx)
#define R3(sc)           (sc->sc_edx)
#define R4(sc)           (sc->sc_edi)
#define R5(sc)           (sc->sc_esi)
#define R6(sc)           (sc->sc_isp)

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, struct sigcontext *sc) \
	{						     			   \
	    sighandler( sig, (regs_t*)sc);             				   \
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

#   if __FreeBSD_version >= 500001
        /*
         * This is the FreeBSD 5.x and higher version
         */
#       define SAVE_FPU(cc,sc)                                              \
            do {                                                            \
                int i_;                                                     \
                (cc)->acc_u.acc_f5.fpformat = (sc)->sc_fpformat;            \
                (cc)->acc_u.acc_f5.ownedfp = (sc)->sc_ownedfp;              \
                for(i_ = 0; i_ < 128; ++i_)                                 \
                    (cc)->acc_u.acc_f5.fpstate[i_] = (sc)->sc_fpstate[i_];  \
            } while (0)

#       define RESTORE_FPU(cc,sc)                                           \
            do {                                                            \
                int i_;                                                     \
                (sc)->sc_fpformat = (cc)->acc_u.acc_f5.fpformat;            \
                (sc)->sc_ownedfp = (cc)->acc_u.acc_f5.ownedfp;              \
                for(i_ = 0; i_ < 128; ++i_)                                 \
                    (sc)->sc_fpstate[i_] = (cc)->acc_u.acc_f5.fpstate[i_];  \
            } while (0)

#       define HAS_FPU(sc)                                                  \
            (                                                               \
                ((sc)->sc_ownedfp != _MC_FPOWNED_NONE)                      \
                &&                                                          \
                ((sc)->sc_fpformat != _MC_FPFMT_NODEV)                      \
            )

#   else

        /*
         * FreeBSD 4 and below have a different context format.
         */
#       define SAVE_FPU(cc,sc)                                              \
            do {                                                            \
                int i_;                                                     \
                for (i_ = 0; i_ < 28; i_++)                                 \
                    (cc)->acc_u.acc_f4.fpstate[i_] = (sc)->sc_fpregs[i_];   \
            } while (0)

#       define RESTORE_FPU(cc,sc)                                           \
            do {                                                            \
                int i_;                                                     \
                for (i_ = 0; i_ < 28; i_++)                                 \
                    (sc)->sc_fpregs[i_] = (cc)->acc_u.acc_f4.fpstate[i_];   \
            } while (0)

#       define HAS_FPU(sc)      0

#   endif

#else
    /* NO FPU VERSION */

#   define SAVE_FPU(cc,sc)                                          \
        do {                                                        \
        } while (0)

#   define RESTORE_FPU(cc,sc)                                       \
        do {                                                        \
        } while (0)

#   define HAS_FPU(sc)      0

#endif

#define SAVEREGS(cc, sc)                                            \
    do {                                                            \
        if (HAS_FPU(sc))                                            \
            SAVE_FPU(cc,sc);                                        \
        SAVE_CPU(cc,sc);                                            \                                          \
        cc->eflags = sc->sc_efl & PSL_USERCHANGE;                   \
    } while (0)

#define RESTOREREGS(cc, sc)                                         \
    do {                                                            \                                       \
	RESTORE_CPU(cc,sc);                                         \
        if (HAS_FPU(sc))                                            \
            RESTORE_FPU(cc,sc);                                     \
        sc->sc_efl = (sc->sc_efl & ~PSL_USERCHANGE) | cc->eflags;   \
    } while (0)

#define PRINT_SC(sc)                                                \
	printf ("    SP=%08lx  FP=%08lx  PC=%08lx  FPU=%s\n"        \
		"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n"      \
		"    R4=%08lx  R5=%08lx  R6=%08lx\n"                \
	    , SP(sc), FP(sc), PC(sc)                                \
	    , HAS_FPU(sc) ? "yes" : "no"                            \
	    , R0(sc), R1(sc), R2(sc), R3(sc)                        \
	    , R4(sc), R5(sc), R6(sc)                                \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

struct AROSCPUContext
{
    ULONG regs[9];	/* eax, ebx, ecx, edx, edi, esi, isp, fp, pc */
    int	errno_backup;
    union
    {
        struct
        {
            int fpformat;
            int ownedfp;
            int fpstate[128] __attribute__ ((aligned (16)));
        }
            acc_f5;

        struct
        {
            int fpstate[17];
        }
            acc_f4;
    }
        acc_u;

    int eflags;
    struct AROSCPUContext *sc;
};

#define GET_PC(ctx) ((APTR)ctx->regs[8])
#define SET_PC(ctx, val) ctx->regs[8] = (ULONG)val

#ifndef NO_FPU

#if __FreeBSD_version >= 500001
/*
 * This is the FreeBSD 5.x and higher version
 */
#define PREPARE_INITIAL_CONTEXT(cc)                                         \
            do {                                                            \
                (cc)->acc_u.acc_f5.fpformat = _MC_FPFMT_NODEV;              \
                (cc)->acc_u.acc_f5.ownedfp = _MC_FPOWNED_NONE;              \
            } while (0)
#else

#define PREPARE_INITIAL_CONTEXT(cc)                                         \
            do {                                                            \
                asm volatile("fninit\n\t"                                   \
                             "fnsave %0\n\t"                                \
                             "fwait" : "=m" ((cc)->acc_u.acc_f4.fpstate)    \
                             );                                             \
            } while (0)

#endif

#define PREPARE_INITIAL_FRAME(cc, sp, startpc)     \
    do {                                           \
        cc->regs[7] = 0;                           \
        cc->regs[8] = (startpc);                   \
    } while (0)
