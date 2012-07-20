/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros to handle unix signals, ARM version
    Lang: english
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

/*
 * WARNING! Initial version, highly untested!!!
 */

#include <exec/types.h>
#include <aros/arm/cpucontext.h>

#ifndef __AROS_EXEC_LIBRARY__

/*
 * This part is included only in host-specific code because it relies
 * on host includes! __AROS_EXEC_LIBRARY__ definition is used to indicate
 * host-independent code
 */

/* We don't use any hacks any more. With modern kernel and libc it's okay */
#define SIGCORE_NEED_SA_SIGINFO 1

#include <signal.h>

#ifdef HOST_OS_android

/*
 * Android NDK doesn't have some necessary includes.
 * Linux kernel is Linux kernel, i hope they won't break binary compatibility,
 * so it's okay to define this structure here.
 */
#include <asm/sigcontext.h>

struct ucontext
{
    unsigned long     uc_flags;
    struct ucontext  *uc_link;
    stack_t           uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t          uc_sigmask;
    int               reserved[32 - (sizeof (sigset_t) / sizeof (int))];
    unsigned long     uc_regspace[128] __attribute__((__aligned__(8)));
};

#else
#include <ucontext.h>
#endif

/* name and type of the signal handler */
#define SIGHANDLER	linux_sighandler
#define SIGHANDLER_T	__sighandler_t

#define GLOBAL_SIGNAL_INIT(sighandler) \
        static void sighandler ## _gate (int sig, siginfo_t *blub, void *u)  \
        {                                                                               \
            sighandler(sig, u);                                                         \
        }

#define SP(uc)	(uc->uc_mcontext.arm_sp)
#define PC(uc)	(uc->uc_mcontext.arm_pc)

/* Macros to enable or disable all signals after the signal handler
   has returned and the normal execution commences. */
#define SC_DISABLE(uc) uc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(uc)  SIGEMPTYSET(&uc->uc_sigmask)

/*
 * Linux kernel does not provide any standarized view of VFP context on signal frame. The ARM linux-hosted
 * port assumes, that the VFP frame is stored in uc_regspace[] area. This is the case on nearly all linux kernel
 * compiled with VFP/NEON support. If this is not the case, or of linux misses the VFP frame in sigcontext, AROS
 * will probably fail.
 */

/*
 * This macro saves all registers. Use this macro when you want to
 * leave the current tasks' context. Note that fpuContext area can
 * be absent, this happens at least in trap handler.
 */
#define SAVEREGS(cc, uc)           							\
do {											\
    CopyMemQuick(&uc->uc_mcontext.arm_r0, (cc)->regs.r, sizeof(ULONG) * 17);		\
    if ((cc)->regs.fpuContext)								\
    {											\
	CopyMemQuick(&uc->uc_regspace[0], (cc)->regs.fpuContext, 128*sizeof(ULONG));	\
	(cc)->regs.Flags |= ECF_FPU;							\
    }											\
} while(0);

/*
 * This macro does the opposite to SAVEREGS(). It restores all
 * registers which are present in the context (depending on its flags).
 * After that, you can enter the new tasks' context.
 */
#define RESTOREREGS(cc, uc)								\
do {											\
    CopyMemQuick((cc)->regs.r, &uc->uc_mcontext.arm_r0, sizeof(ULONG) * 17);		\
    if ((cc)->regs.Flags & ECF_FPU)							\
	CopyMemQuick((cc)->regs.fpuContext, &uc->uc_regspace[0], 128*sizeof(ULONG));	\
} while(0);

/* Print signal context. Used in crash handler */
#define PRINT_SC(sc) \
    bug ("    R0=%08X  R1=%08X  R2 =%08X  R3 =%08X\n" \
    	 "    R4=%08X  R5=%08X  R6 =%08X  R7 =%08X\n" \
    	 "    R8=%08X  R9=%08X  R10=%08X  R11=%08X\n" \
    	 "    IP=%08X  SP=%08X  LR =%08X  PC =%08X\n" \
    	 "    CPSR=%08X\n"			      \
	    , (sc)->uc_mcontext.arm_r0, (sc)->uc_mcontext.arm_r1, (sc)->uc_mcontext.arm_r2, (sc)->uc_mcontext.arm_r3	\
	    , (sc)->uc_mcontext.arm_r4, (sc)->uc_mcontext.arm_r5, (sc)->uc_mcontext.arm_r6, (sc)->uc_mcontext.arm_r7	\
    	    , (sc)->uc_mcontext.arm_r8, (sc)->uc_mcontext.arm_r9, (sc)->uc_mcontext.arm_r10, (sc)->uc_mcontext.arm_fp	\
	    , (sc)->uc_mcontext.arm_ip, (sc)->uc_mcontext.arm_sp, (sc)->uc_mcontext.arm_lr, (sc)->uc_mcontext.arm_pc	\
	    , (sc)->uc_mcontext.arm_cpsr										\
	)

#endif /* __AROS_EXEC_LIBRARY__ */

/* We emulate 6 exceptions of ARM CPU (all but softint) */
#define EXCEPTIONS_COUNT 6

typedef struct ucontext regs_t;

/* This structure is used to save/restore registers */
struct AROSCPUContext
{
    struct ExceptionContext regs;		/* Public portion	       */
    int		            errno_backup;	/* Host-specific magic follows */
};

/*
 * ARM FPU context under Linux is currently private.
 * So we set type to FPU_NONE (so that noone tries to mess with it).
 */
#define ARM_FPU_TYPE FPU_NONE
#define ARM_FPU_SIZE (128*sizeof(ULONG))

#endif /* _SIGCORE_H */
