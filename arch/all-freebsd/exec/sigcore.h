/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <machine/psl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ucontext.h>
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

/*
 * We can't have an #ifdef based on FreeBSD here because this structure
 * is (wrongly) accessed from rom/exec.
 */
struct AROS_cpu_context
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
	    sighandler( sig, (sigcontext_t*)sc);             \
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

#define SAVE_ERRNO(cc)                                              \
    do {                                                            \
	(cc)->errno_backup = errno;                                 \
    } while (0)

#define RESTORE_ERRNO(cc)                                           \
    do {                                                            \
	errno = (cc)->errno_backup;                                 \
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

#       define PREPARE_FPU(cc)                                              \
            do {                                                            \
                (cc)->acc_u.acc_f5.fpformat = _MC_FPFMT_NODEV;              \
                (cc)->acc_u.acc_f5.ownedfp = _MC_FPOWNED_NONE;              \
            } while (0)

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

#       define PREPARE_FPU(cc)                                              \
            do {                                                            \
                asm volatile("fninit\n\t"                                   \
                             "fnsave %0\n\t"                                \
                             "fwait" : "=m" ((cc)->acc_u.acc_f4.fpstate)    \
                             );                                             \
            } while (0)

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

#   define PREPARE_FPU(cc)                                          \
        do {                                                        \
        } while (0)

#endif

#define PREPARE_INITIAL_FRAME(sp,startpc)                           \
    do {                                                            \
        GetCpuContext(task)->regs[7] = 0;                           \
        GetCpuContext(task)->regs[8] = (startpc);                   \
    } while (0)

#define PREPARE_INITIAL_CONTEXT(task,startpc)                       \
    do {                                                            \
        PREPARE_FPU(GetCpuContext(task));                           \
    } while (0)

#define SAVEREGS(task,sc)                                           \
    do {                                                            \
        struct AROS_cpu_context *cc = GetCpuContext(task);          \
        GetSP(task) = (SP_TYPE *)SP(sc);                            \
        if (HAS_FPU(sc))                                            \
            SAVE_FPU(cc,sc);                                        \
        SAVE_CPU(cc,sc);                                            \
        SAVE_ERRNO(cc);                                             \
        cc->eflags = sc->sc_efl & PSL_USERCHANGE;                   \
    } while (0)

#define RESTOREREGS(task,sc)                                        \
    do {                                                            \
        struct AROS_cpu_context *cc = GetCpuContext(task);          \
	RESTORE_ERRNO(cc);                                          \
	RESTORE_CPU(cc,sc);                                         \
        if (HAS_FPU(sc))                                            \
            RESTORE_FPU(cc,sc);                                     \
	SP(sc) = (SP_TYPE *)GetSP(task);                            \
    sc->sc_efl = (sc->sc_efl & ~PSL_USERCHANGE) | cc->eflags;   \
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
