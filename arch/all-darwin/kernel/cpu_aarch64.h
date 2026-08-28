/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU context definitions for AArch64 Darwin (macOS Apple Silicon) hosted
*/

#include <exec/types.h>
#include <aros/aarch64/cpucontext.h>

#ifdef __AROS_EXEC_LIBRARY__

/* regs_t is a black box here */
struct ucontext;
typedef struct ucontext *regs_t;

#else

#include <sys/ucontext.h>

#define SIGCORE_NEED_SA_SIGINFO

typedef ucontext_t regs_t;

#define SIGHANDLER	bsd_sighandler
typedef void (*SIGHANDLER_T)(int);

#define SC_DISABLE(sc)   sc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(sc)						\
do {							\
    pd->iface->SigEmptySet(&(sc)->uc_sigmask);		\
    AROS_HOST_BARRIER					\
} while(0)

/*
 * macOS arm64 mcontext register access.
 *
 * _STRUCT_ARM_THREAD_STATE64 (non-opaque, __DARWIN_UNIX03):
 *   __uint64_t __x[29]   — General purpose registers x0-x28
 *   __uint64_t __fp       — Frame pointer x29
 *   __uint64_t __lr       — Link register x30
 *   __uint64_t __sp       — Stack pointer
 *   __uint64_t __pc       — Program counter
 *   __uint32_t __cpsr     — Current program status register
 *   __uint32_t __pad
 *
 * _STRUCT_ARM_NEON_STATE64:
 *   __uint128_t __v[32]   — NEON/FP registers V0-V31
 *   __uint32_t  __fpsr    — FP status register
 *   __uint32_t  __fpcr    — FP control register
 */

#if __DARWIN_UNIX03

#define Xn(context, n)  ((context)->uc_mcontext->__ss.__x[(n)])
#define FP(context)     ((context)->uc_mcontext->__ss.__fp)
#define LR(context)     ((context)->uc_mcontext->__ss.__lr)
#define SP(context)     ((context)->uc_mcontext->__ss.__sp)
#define PC(context)     ((context)->uc_mcontext->__ss.__pc)
#define CPSR(context)   ((context)->uc_mcontext->__ss.__cpsr)

#define GPSTATE(context) ((context)->uc_mcontext->__ss)
#define FPSTATE(context) ((context)->uc_mcontext->__ns)

#else /* !__DARWIN_UNIX03 */

#define Xn(context, n)  ((context)->uc_mcontext->ss.x[(n)])
#define FP(context)     ((context)->uc_mcontext->ss.fp)
#define LR(context)     ((context)->uc_mcontext->ss.lr)
#define SP(context)     ((context)->uc_mcontext->ss.sp)
#define PC(context)     ((context)->uc_mcontext->ss.pc)
#define CPSR(context)   ((context)->uc_mcontext->ss.cpsr)

#define GPSTATE(context) ((context)->uc_mcontext->ss)
#define FPSTATE(context) ((context)->uc_mcontext->ns)

#endif /* __DARWIN_UNIX03 */

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, siginfo_t *info, void *sc) \
	{						     \
	    sighandler(sig, sc);			     \
	}

/*
 * SAVEREGS: Copy Darwin signal context → AROS ExceptionContext.
 *
 * The layout of _STRUCT_ARM_THREAD_STATE64 matches our ExceptionContext
 * for x0-x28, fp, lr — they are contiguous uint64_t fields in both.
 * sp, pc, and cpsr/pstate need individual handling since Darwin's struct
 * has cpsr as uint32_t + pad while ours has pstate as IPTR.
 */
#define SAVEREGS(cc, sc)                                                    \
do {                                                                        \
    int __i;                                                                \
    for (__i = 0; __i < 29; __i++)                                          \
        (cc)->regs.r[__i] = Xn(sc, __i);                                   \
    (cc)->regs.fp = FP(sc);                                                 \
    (cc)->regs.lr = LR(sc);                                                 \
    (cc)->regs.sp = SP(sc);                                                 \
    (cc)->regs.pc = PC(sc);                                                 \
    (cc)->regs.pstate = CPSR(sc);                                           \
    if ((cc)->regs.fpuContext)                                               \
    {                                                                       \
        (cc)->regs.Flags |= ECF_FPU;                                        \
        CopyMemQuick(&FPSTATE(sc), (cc)->regs.fpuContext,                   \
                     sizeof(struct VFPContext));                             \
    }                                                                       \
} while (0)

#define RESTOREREGS(cc, sc)                                                 \
do {                                                                        \
    int __i;                                                                \
    for (__i = 0; __i < 29; __i++)                                          \
        Xn(sc, __i) = (cc)->regs.r[__i];                                   \
    FP(sc) = (cc)->regs.fp;                                                 \
    LR(sc) = (cc)->regs.lr;                                                 \
    SP(sc) = (cc)->regs.sp;                                                 \
    PC(sc) = (cc)->regs.pc;                                                 \
    CPSR(sc) = (uint32_t)(cc)->regs.pstate;                                 \
    if ((cc)->regs.Flags & ECF_FPU)                                          \
        CopyMemQuick((cc)->regs.fpuContext, &FPSTATE(sc),                   \
                     sizeof(struct VFPContext));                             \
} while (0)

/* Print signal context. Used in crash handler. */
#define PRINT_SC(sc) \
    bug ("    X0 =%016lx  X1 =%016lx  X2 =%016lx  X3 =%016lx\n"  \
         "    X4 =%016lx  X5 =%016lx  X6 =%016lx  X7 =%016lx\n"  \
         "    X8 =%016lx  X9 =%016lx  X10=%016lx  X11=%016lx\n"  \
         "    X12=%016lx  X13=%016lx  X14=%016lx  X15=%016lx\n"  \
         "    X16=%016lx  X17=%016lx  X18=%016lx  X19=%016lx\n"  \
         "    X20=%016lx  X21=%016lx  X22=%016lx  X23=%016lx\n"  \
         "    X24=%016lx  X25=%016lx  X26=%016lx  X27=%016lx\n"  \
         "    X28=%016lx  FP =%016lx  LR =%016lx  SP =%016lx\n"  \
         "    PC =%016lx  CPSR=%08x\n"                             \
        , Xn(sc,0),  Xn(sc,1),  Xn(sc,2),  Xn(sc,3)              \
        , Xn(sc,4),  Xn(sc,5),  Xn(sc,6),  Xn(sc,7)              \
        , Xn(sc,8),  Xn(sc,9),  Xn(sc,10), Xn(sc,11)             \
        , Xn(sc,12), Xn(sc,13), Xn(sc,14), Xn(sc,15)             \
        , Xn(sc,16), Xn(sc,17), Xn(sc,18), Xn(sc,19)             \
        , Xn(sc,20), Xn(sc,21), Xn(sc,22), Xn(sc,23)             \
        , Xn(sc,24), Xn(sc,25), Xn(sc,26), Xn(sc,27)             \
        , Xn(sc,28), FP(sc),    LR(sc),    SP(sc)                 \
        , PC(sc),    CPSR(sc)                                      \
    )

#endif /* __AROS_EXEC_LIBRARY__ */

/* AArch64 has the same exception classes as ARM32 for hosted purposes */
#define EXCEPTIONS_COUNT 6

struct AROSCPUContext
{
    struct ExceptionContext regs;
    int errno_backup;
};

/* AArch64 always has NEON/VFP */
#define ARM_FPU_TYPE FPU_VFP
#define ARM_FPU_SIZE sizeof(struct VFPContext)
