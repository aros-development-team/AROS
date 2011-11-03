/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

#include <aros/i386/cpucontext.h>

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
#define SC_ENABLE(sc)    pd->iface->SigEmptySet(&(sc)->uc_sigmask)

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
#define PC(context)     ((context)->uc_mcontext->__ss.__eip)
#define SP(context)     ((context)->uc_mcontext->__ss.__esp)

#define SS(context)	((context)->uc_mcontext->__ss.__ss)
#define CS(context)	((context)->uc_mcontext->__ss.__cs)
#define DS(context)	((context)->uc_mcontext->__ss.__ds)
#define ES(context)	((context)->uc_mcontext->__ss.__es)
#define FS(context)	((context)->uc_mcontext->__ss.__fs)
#define GS(context)	((context)->uc_mcontext->__ss.__gs)

#define FPSTATE(context) ((context)->uc_mcontext->__fs.__fpu_fcw)

#else

#define R0(context)     ((context)->uc_mcontext->ss.eax)
#define R1(context)     ((context)->uc_mcontext->ss.ebx)
#define R2(context)     ((context)->uc_mcontext->ss.ecx)
#define R3(context)     ((context)->uc_mcontext->ss.edx)
#define R4(context)     ((context)->uc_mcontext->ss.edi)
#define R5(context)     ((context)->uc_mcontext->ss.esi)
#define R6(context)     ((context)->uc_mcontext->ss.eflags)

#define FP(context)     ((context)->uc_mcontext->ss.ebp)
#define PC(context)     ((context)->uc_mcontext->ss.eip)
#define SP(context)     ((context)->uc_mcontext->ss.esp)

#define SS(context)	((context)->uc_mcontext->ss.ss)
#define CS(context)	((context)->uc_mcontext->ss.cs)
#define DS(context)	((context)->uc_mcontext->ss.ds)
#define ES(context)	((context)->uc_mcontext->ss.es)
#define FS(context)	((context)->uc_mcontext->ss.fs)
#define GS(context)	((context)->uc_mcontext->ss.gs)

#define FPSTATE(context) ((context)->uc_mcontext->fs.fpu_fcw)

#endif

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, ucontext_t *sc) \
	{						     \
	    sighandler(sig, sc);		             \
	}

#define SAVE_CPU(cc, sc)	\
    cc.Flags = ECF_SEGMENTS;	\
    cc.eax    = R0(sc);		\
    cc.ebx    = R1(sc);		\
    cc.ecx    = R2(sc);		\
    cc.edx    = R3(sc);		\
    cc.edi    = R4(sc);		\
    cc.esi    = R5(sc);		\
    cc.eflags = R6(sc);		\
    cc.ebp    = FP(sc);		\
    cc.eip    = PC(sc);		\
    cc.esp    = SP(sc);		\
    cc.cs     = CS(sc);		\
    cc.ds     = DS(sc);		\
    cc.es     = ES(sc);		\
    cc.fs     = FS(sc);		\
    cc.gs     = GS(sc);		\
    cc.ss     = SS(sc);

/*
 * Restore CPU registers.
 * Note that we do not restore segment registers because they
 * are of own use by Darwin.
 */
#define RESTORE_CPU(cc, sc) \
    R0(sc) = cc.eax;        \
    R1(sc) = cc.ebx;        \
    R2(sc) = cc.ecx;        \
    R3(sc) = cc.edx;        \
    R4(sc) = cc.edi;        \
    R5(sc) = cc.esi;        \
    R6(sc) = cc.eflags;     \
    FP(sc) = cc.ebp;        \
    PC(sc) = cc.eip;        \
    SP(sc) = cc.esp;

/*
 * Save all registers from UNIX signal context ss to AROS context cc.
 * Saves SSE state only if the context has appropriate space for it.
 * Note that Darwin does not save legacy 8087 frame.
 */
#define SAVEREGS(cc, sc)                                       				\
    SAVE_CPU((cc)->regs, sc);								\
    if ((cc)->regs.FXData)								\
    {											\
    	(cc)->regs.Flags |= ECF_FPX;							\
    	CopyMemQuick(&FPSTATE(sc), (cc)->regs.FXData, sizeof(struct FPXContext));	\
    }

/*
 * Restore all registers from AROS context to UNIX signal context.
 * Check context flags to decide whether to restore SSE or not.
 */
#define RESTOREREGS(cc, sc)                                    				\
    RESTORE_CPU((cc)->regs, sc);							\
    if ((cc)->regs.Flags & ECF_FPX)							\
	CopyMemQuick((cc)->regs.FXData, &FPSTATE(sc), sizeof(struct FPXContext));

/* Print signal context. Used in crash handler. */
#define PRINT_SC(sc) \
    bug ("    ESP=%08x  EBP=%08x  EIP=%08x\n" \
	 "    EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n" \
	 "    EDI=%08x  ESI=%08x  EFLAGS=%08x\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 17

struct AROSCPUContext
{
    struct ExceptionContext regs;
    int errno_backup;
};
