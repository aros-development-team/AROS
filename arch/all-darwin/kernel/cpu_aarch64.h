/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Darwin hosted host-kernel CPU glue for AArch64, the AArch64 peer of
    cpu_arm.h / cpu_x86_64.h. Maps a Darwin arm64 signal context (ucontext)
    to/from struct ExceptionContext, so the hosted "interrupt" (a Unix signal)
    can save and resume AROS tasks. Host structures per the SDK:
    <mach/arm/_structs.h> (_STRUCT_ARM_THREAD_STATE64) and <arm/_mcontext.h>
    (_STRUCT_MCONTEXT64 = { __es, __ss, __ns }).
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

#define SIGHANDLER      bsd_sighandler
typedef void (*SIGHANDLER_T)(int);

#define SC_DISABLE(sc)   sc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(sc)                           \
do {                                            \
    pd->iface->SigEmptySet(&(sc)->uc_sigmask);  \
    AROS_HOST_BARRIER                           \
} while(0)

/*
 * For -arch arm64, __DARWIN_OPAQUE_ARM_THREAD_STATE64 == 0, so the thread state
 * exposes the plain register fields (no pointer-auth opaque packing).
 * __ss = general state, __ns = NEON/FP state.
 */
#define Xn(context, n)  ((context)->uc_mcontext->__ss.__x[(n)])
#define FP(context)     ((context)->uc_mcontext->__ss.__fp)   /* x29 */
#define LR(context)     ((context)->uc_mcontext->__ss.__lr)   /* x30 */
#define SP(context)     ((context)->uc_mcontext->__ss.__sp)   /* x31 */
#define PC(context)     ((context)->uc_mcontext->__ss.__pc)
#define CPSR(context)   ((context)->uc_mcontext->__ss.__cpsr)

/* Exception state: faulting virtual address + exception syndrome. Defining
 * FAULTADDR also signals the shared trap handler (arch/all-unix/kernel/kernel.c)
 * to dump the fault address + GPRs -- skipped on arches that don't define it. */
#define FAULTADDR(context) ((context)->uc_mcontext->__es.__far)
#define ESR(context)       ((context)->uc_mcontext->__es.__esr)

#define GPSTATE(context) ((context)->uc_mcontext->__ss)
#define FPSTATE(context) ((context)->uc_mcontext->__ns)

#define GLOBAL_SIGNAL_INIT(sighandler) \
    static void sighandler ## _gate (int sig, siginfo_t *info, void *sc) \
    {                                                                    \
        sighandler(sig, sc);                                             \
    }

/*
 * SAVEREGS / RESTOREREGS rely on struct ExceptionContext following the
 * architectural register order (x[29], fp, lr, sp, pc, cpsr), matching
 * Darwin's _STRUCT_ARM_THREAD_STATE64; see <aros/aarch64/cpucontext.h>.
 *
 * Copy only the integer register state (x0-x28, fp, lr, sp, pc, cpsr), i.e. up to
 * -- but NOT including -- the AROS-private Flags word. _STRUCT_ARM_THREAD_STATE64
 * ends with __cpsr(u32)+__pad(u32); struct ExceptionContext has cpsr(u32) at the
 * same offset but then Flags(u32). Copying the full sizeof(_STRUCT_ARM_THREAD_
 * STATE64) would overwrite Flags with the host __pad -- and a stray ECF_FPU bit
 * there makes RESTOREREGS deref a NULL fpuContext. So bound the copy at Flags.
 */
#define AARCH64_GPREGS_SIZE __builtin_offsetof(struct ExceptionContext, Flags)

#define SAVEREGS(cc, sc)                                                          \
    CopyMemQuick(&GPSTATE(sc), (cc)->regs.x, AARCH64_GPREGS_SIZE);                 \
    if ((cc)->regs.fpuContext)                                                    \
    {                                                                            \
        (cc)->regs.Flags |= ECF_FPU;                                             \
        CopyMemQuick(&FPSTATE(sc), (cc)->regs.fpuContext, sizeof(_STRUCT_ARM_NEON_STATE64)); \
    }

#define RESTOREREGS(cc, sc)                                                       \
    CopyMemQuick((cc)->regs.x, &GPSTATE(sc), AARCH64_GPREGS_SIZE);                 \
    if ((cc)->regs.Flags & ECF_FPU)                                              \
        CopyMemQuick((cc)->regs.fpuContext, &FPSTATE(sc), sizeof(_STRUCT_ARM_NEON_STATE64));

/* Print signal context (all general-purpose registers). Used in the crash
 * handler -- the faulting value often lives in a callee-saved register, so dump
 * the full x0-x28 set plus fp(x29)/lr(x30)/sp/pc/cpsr. */
#define PRINT_SC(sc) \
    bug ("    X0 =%016llX X1 =%016llX X2 =%016llX X3 =%016llX\n" \
         "    X4 =%016llX X5 =%016llX X6 =%016llX X7 =%016llX\n" \
         "    X8 =%016llX X9 =%016llX X10=%016llX X11=%016llX\n" \
         "    X12=%016llX X13=%016llX X14=%016llX X15=%016llX\n" \
         "    X16=%016llX X17=%016llX X18=%016llX X19=%016llX\n" \
         "    X20=%016llX X21=%016llX X22=%016llX X23=%016llX\n" \
         "    X24=%016llX X25=%016llX X26=%016llX X27=%016llX\n" \
         "    X28=%016llX FP =%016llX LR =%016llX SP =%016llX\n" \
         "    PC =%016llX CPSR=%08X\n"                           \
            , (unsigned long long)Xn(sc,0),  (unsigned long long)Xn(sc,1)  \
            , (unsigned long long)Xn(sc,2),  (unsigned long long)Xn(sc,3)  \
            , (unsigned long long)Xn(sc,4),  (unsigned long long)Xn(sc,5)  \
            , (unsigned long long)Xn(sc,6),  (unsigned long long)Xn(sc,7)  \
            , (unsigned long long)Xn(sc,8),  (unsigned long long)Xn(sc,9)  \
            , (unsigned long long)Xn(sc,10), (unsigned long long)Xn(sc,11) \
            , (unsigned long long)Xn(sc,12), (unsigned long long)Xn(sc,13) \
            , (unsigned long long)Xn(sc,14), (unsigned long long)Xn(sc,15) \
            , (unsigned long long)Xn(sc,16), (unsigned long long)Xn(sc,17) \
            , (unsigned long long)Xn(sc,18), (unsigned long long)Xn(sc,19) \
            , (unsigned long long)Xn(sc,20), (unsigned long long)Xn(sc,21) \
            , (unsigned long long)Xn(sc,22), (unsigned long long)Xn(sc,23) \
            , (unsigned long long)Xn(sc,24), (unsigned long long)Xn(sc,25) \
            , (unsigned long long)Xn(sc,26), (unsigned long long)Xn(sc,27) \
            , (unsigned long long)Xn(sc,28)                               \
            , (unsigned long long)FP(sc),  (unsigned long long)LR(sc)     \
            , (unsigned long long)SP(sc),  (unsigned long long)PC(sc)     \
            , (unsigned int)CPSR(sc)                                      \
        )

#endif /* __AROS_EXEC_LIBRARY__ */

/* We emulate the AArch64 synchronous/IRQ/FIQ/SError exceptions (not softint). */
#define EXCEPTIONS_COUNT 6

/*
 * FP/NEON save area size. A raw byte blob, NOT _STRUCT_ARM_NEON_STATE64,
 * because kernel.resource TUs build with __AROS_EXEC_LIBRARY__ (no Apple
 * headers, regs_t is a black box) yet still need struct AROSCPUContext.
 * 576 >= sizeof(_STRUCT_ARM_NEON_STATE64) (520 -> 528 padded); checked at
 * compile time below where the real type is visible.
 */
#define AARCH64_FPU_AREA_SIZE 576

struct AROSCPUContext
{
    struct ExceptionContext regs;
    int errno_backup;
    /*
     * FP/NEON save area. regs.fpuContext must point here (set by
     * PREPARE_INITIAL_CONTEXT below, via KrnCreateContext) or SAVEREGS/
     * RESTOREREGS silently skip q0-q31/fpsr/fpcr and every task switch leaks
     * one task's NEON state into the next. On-stack AROSCPUContext users
     * (core_TrapHandler) memset() the struct, leaving fpuContext NULL --
     * there the skip is intentional and safe.
     */
    unsigned char fpu[AARCH64_FPU_AREA_SIZE] __attribute__((aligned(16)));
};

#define PREPARE_INITIAL_CONTEXT(ctx) ((ctx)->regs.fpuContext = (ctx)->fpu)

/* Darwin arm64 has NEON/VFP. */
#define AARCH64_FPU_TYPE FPU_VFP
#define AARCH64_FPU_SIZE sizeof(_STRUCT_ARM_NEON_STATE64)

#ifndef __AROS_EXEC_LIBRARY__
/* The raw save area must hold the real NEON state (only checkable where the
 * Apple type is visible). */
typedef char __aarch64_fpu_area_fits[(AARCH64_FPU_AREA_SIZE >= (int)sizeof(_STRUCT_ARM_NEON_STATE64)) ? 1 : -1];
#endif
