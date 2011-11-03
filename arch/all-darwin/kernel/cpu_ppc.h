/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/ppc/cpucontext.h>

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
#define SC_ENABLE(sc)	 pd->iface->SigEmptySet(&(sc)->uc_sigmask)

/* work around silly renaming of struct members in OS X 10.5 */
#if __DARWIN_UNIX03

#define DAR(context)	 ((context)->uc_mcontext->__es.__dar)
#define DSISR(context)	 ((context)->uc_mcontext->__es.__dsisr)

#define PC(context)	 ((context)->uc_mcontext->__ss.__srr0)
#define SRR1(context)	 ((context)->uc_mcontext->__ss.__srr1)
#define R0(context)      ((context)->uc_mcontext->__ss.__r0)
#define SP(context)      ((context)->uc_mcontext->__ss.__r1)
#define R2(context)      ((context)->uc_mcontext->__ss.__r2)
#define R3(context)      ((context)->uc_mcontext->__ss.__r3)
#define R4(context)      ((context)->uc_mcontext->__ss.__r4)
#define R5(context)      ((context)->uc_mcontext->__ss.__r5)
#define R6(context)      ((context)->uc_mcontext->__ss.__r6)
#define R7(context)      ((context)->uc_mcontext->__ss.__r7)
#define R8(context)      ((context)->uc_mcontext->__ss.__r8)
#define R9(context)      ((context)->uc_mcontext->__ss.__r9)
#define R10(context)     ((context)->uc_mcontext->__ss.__r10)
#define R11(context)     ((context)->uc_mcontext->__ss.__r11)
#define R12(context)     ((context)->uc_mcontext->__ss.__r12)
#define CR(context)      ((context)->uc_mcontext->__ss.__cr)
#define XER(context)     ((context)->uc_mcontext->__ss.__xer)
#define CTR(context)     ((context)->uc_mcontext->__ss.__ctr)
#define LR(context)      ((context)->uc_mcontext->__ss.__lr)
#define VRSAVE(context)	 ((context)->uc_mcontext->__ss.__vrsave)

#define FPSCR(context)	 ((context)->uc_mcontext->__fs.__fpscr)

#define VR(context)	 ((context)->uc_mcontext->__vs.__save_vr)
#define VSCR(context)	 ((context)->uc_mcontext->__vs.__save_vscr)
#define VRVALID(context) ((context)->uc_mcontext->__vs.__save_vrvalid)

#define FPSTATE(context) ((context)->uc_mcontext->__fs)

#else

#define DAR(context)	 ((context)->uc_mcontext->es.dar)
#define DSISR(context)	 ((context)->uc_mcontext->es.dsisr)

#define PC(context)	 ((context)->uc_mcontext->ss.srr0)
#define SRR1(context)	 ((context)->uc_mcontext->ss.srr1)
#define R0(context)      ((context)->uc_mcontext->ss.r0)
#define SP(context)      ((context)->uc_mcontext->ss.r1)
#define R2(context)      ((context)->uc_mcontext->ss.r2)
#define R3(context)      ((context)->uc_mcontext->ss.r3)
#define R4(context)      ((context)->uc_mcontext->ss.r4)
#define R5(context)      ((context)->uc_mcontext->ss.r5)
#define R6(context)      ((context)->uc_mcontext->ss.r6)
#define R7(context)      ((context)->uc_mcontext->ss.r7)
#define R8(context)      ((context)->uc_mcontext->ss.r8)
#define R9(context)      ((context)->uc_mcontext->ss.r9)
#define R10(context)     ((context)->uc_mcontext->ss.r10)
#define R11(context)     ((context)->uc_mcontext->ss.r11)
#define R12(context)     ((context)->uc_mcontext->ss.r12)
#define CR(context)      ((context)->uc_mcontext->ss.cr)
#define XER(context)     ((context)->uc_mcontext->ss.xer)
#define CTR(context)     ((context)->uc_mcontext->ss.ctr)
#define LR(context)      ((context)->uc_mcontext->ss.lr)
#define VRSAVE(context)	 ((context)->uc_mcontext->ss.vrsave)

#define FPSCR(context)	 ((context)->uc_mcontext->fs.fpscr)

#define VR(context)	 ((context)->uc_mcontext->vs.save_vr)
#define VSCR(context)	 ((context)->uc_mcontext->vs.save_vscr)
#define VRVALID(context) ((context)->uc_mcontext->vs.save_vrvalid)

#define FPSTATE(context) ((context)->uc_mcontext->fs)

#endif 

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, ucontext_t *sc) \
	{						     \
	    sighandler(sig, sc);		             \
	}

/*
 * SAVEREGS and RESTOREREGS rely on the fact that layout of some parts of
 * struct ExceptionContext (r0 - xer and FPU state) is the same as layout
 * of these parts in Darwin's context.
 */
#define SAVEREGS(cc, sc)							\
    (cc)->regs.Flags  = ECF_FULL_GPRS|ECF_FPU|ECF_FULL_FPU|ECF_VRSAVE;		\
    (cc)->regs.msr    = SRR1(sc);						\
    (cc)->regs.ip     = PC(sc);							\
    CopyMemQuick(&R0(sc), (cc)->regs.gpr, 34 * sizeof(ULONG));			\
    (cc)->regs.ctr    = CTR(sc);						\
    (cc)->regs.lr     = LR(sc);							\
    (cc)->regs.dsisr  = DSISR(sc);						\
    (cc)->regs.dar    = DAR(sc);						\
    CopyMemQuick(&FPSTATE(sc), (cc)->regs.fpr, sizeof(_STRUCT_PPC_FLOAT_STATE)); \
    (cc)->regs.vrsave = VRSAVE(sc);						\
    if (VRVALID(sc))								\
    {										\
    	(cc)->regs.Flags |= ECF_VECTOR;						\
    	CopyMemQuick(VR(sc), (cc)->regs.vr, 512);				\
    	CopyMemQuick(VSCR(sc), (cc)->regs.vscr, 16);				\
    }

#define RESTOREREGS(cc, sc)                                         		\
{										\
    ULONG n    = ((cc)->regs.Flags & ECF_FULL_GPRS) ? 32 : 14;			\
    SRR1(sc)   = (cc)->regs.msr;						\
    PC(sc)     = (cc)->regs.ip;							\
    CopyMemQuick((cc)->regs.gpr, &R0(sc), n * sizeof(ULONG));			\
    CR(sc)     = (cc)->regs.cr;							\
    XER(sc)    = (cc)->regs.xer;						\
    CTR(sc)    = (cc)->regs.ctr;						\
    LR(sc)     = (cc)->regs.lr;							\
    DSISR(sc)  = (cc)->regs.dsisr;						\
    DAR(sc)    = (cc)->regs.dar;						\
    if ((cc)->regs.Flags & ECF_FPU)						\
	CopyMemQuick(&FPSTATE(sc), (cc)->regs.fpr, 32 * sizeof(double));	\
    if ((cc)->regs.Flags & ECF_FULL_FPU)					\
    	FPSCR(sc)  = (cc)->regs.fpscr;						\
    if ((cc)->regs.Flags & ECF_VRSAVE)						\
    	VRSAVE(sc) = (cc)->regs.vrsave;						\
    if ((cc)->regs.Flags & ECF_VECTOR)						\
    {										\
    	CopyMemQuick((cc)->regs.vr, VR(sc), 512);				\
    	CopyMemQuick((cc)->regs.vscr, VSCR(sc), 16);				\
    }										\
}

/* Print signal context. Used in crash handler */
#define PRINT_SC(sc)					\
    bug ("    R0 =%08X  R1=%08X  R2 =%08X  R3 =%08X\n"	\
    	 "    R4 =%08X  R5=%08X  R6 =%08X  R7 =%08X\n"	\
    	 "    R8 =%08X  R9=%08X  R10=%08X  R11=%08X\n"	\
    	 "    R12=%08X  LR=%08X  MSR=%08X  IP =%08X\n"	\
	    , R0(sc), SP(sc), R2(sc), R3(sc)		\
	    , R4(sc), R5(sc), R6(sc), R7(sc)		\
	    , R8(sc), R9(sc), R10(sc), R11(sc)		\
	    , R12(sc), LR(sc), SRR1(sc), PC(sc)	\
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 14

struct AROSCPUContext
{
    struct ExceptionContext regs;
    int	errno_backup;
};
