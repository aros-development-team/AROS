/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: cpu_i386.h 35261 2010-11-08 08:41:28Z sonic $
*/

#include <exec/types.h>
#include <aros/arm/cpucontext.h>

#ifdef __AROS_EXEC_LIBRARY__

/* regs_t is a black box here */
struct ucontext;
typedef struct ucontext *regs_t;

#else

#include <sys/ucontext.h>

typedef ucontext_t regs_t;

#define SIGHANDLER	bsd_sighandler
typedef void (*SIGHANDLER_T)(int);

#define SC_DISABLE(sc)   sc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(sc)    KernelIFace.SigEmptySet(&(sc)->uc_sigmask)

/* work around silly renaming of struct members in OS X 10.5 */
#if __DARWIN_UNIX03

#define R0(context)     ((context)->uc_mcontext->__ss.__r[0])
#define R1(context)     ((context)->uc_mcontext->__ss.__r[1])
#define R2(context)     ((context)->uc_mcontext->__ss.__r[2])
#define R3(context)     ((context)->uc_mcontext->__ss.__r[3])
#define R4(context)     ((context)->uc_mcontext->__ss.__r[4])
#define R5(context)     ((context)->uc_mcontext->__ss.__r[5])
#define R6(context)     ((context)->uc_mcontext->__ss.__r[6])
#define R7(context)     ((context)->uc_mcontext->__ss.__r[7])
#define R8(context)     ((context)->uc_mcontext->__ss.__r[8])
#define R9(context)     ((context)->uc_mcontext->__ss.__r[9])
#define R10(context)    ((context)->uc_mcontext->__ss.__r[10])
#define R11(context)    ((context)->uc_mcontext->__ss.__r[11])
#define R12(context)    ((context)->uc_mcontext->__ss.__r[12])
#define SP(context)     ((context)->uc_mcontext->__ss.__sp)
#define LR(context)     ((context)->uc_mcontext->__ss.__lr)
#define PC(context)     ((context)->uc_mcontext->__ss.__pc)

#define GPSTATE(context) ((context)->uc_mcontext->__ss)
#define FPSTATE(context) ((context)->uc_mcontext->__fs)

#else

#define R0(context)     ((context)->uc_mcontext->ss.r[0])
#define R1(context)     ((context)->uc_mcontext->ss.r[1])
#define R2(context)     ((context)->uc_mcontext->ss.r[2])
#define R3(context)     ((context)->uc_mcontext->ss.r[3])
#define R4(context)     ((context)->uc_mcontext->ss.r[4])
#define R5(context)     ((context)->uc_mcontext->ss.r[5])
#define R6(context)     ((context)->uc_mcontext->ss.r[6])
#define R7(context)     ((context)->uc_mcontext->ss.r[7])
#define R8(context)     ((context)->uc_mcontext->ss.r[8])
#define R9(context)     ((context)->uc_mcontext->ss.r[9])
#define R10(context)    ((context)->uc_mcontext->ss.r[10])
#define R11(context)    ((context)->uc_mcontext->ss.r[11])
#define R12(context)    ((context)->uc_mcontext->ss.r[12])
#define SP(context)     ((context)->uc_mcontext->ss.sp)
#define LR(context)     ((context)->uc_mcontext->ss.lr)
#define PC(context)     ((context)->uc_mcontext->ss.pc)

#define GPSTATE(context) ((context)->uc_mcontext->__ss)
#define FPSTATE(context) ((context)->uc_mcontext->fs)

#endif 

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, ucontext_t *sc) \
	{						     \
	    sighandler(sig, sc);		             \
	}

/*
 * SAVEREGS and RESTOREREGS rely on the fact that layout of
 * struct ExceptionContext is actually the same as layout of
 * Darwin's context.
 *
 * Note that we assume that VFP area is always present in our context.
 * In fact it should (Darwin seems not to support anything else than VFP)
 */
#define SAVEREGS(cc, sc)							\
    CopyMemQuick(&GPSTATE(sc), cc->regs.r, sizeof(_STRUCT_ARM_THREAD_STATE));	\
    CopyMemQuick(&FPSTATE(sc), cc->fpuContext, sizeof(struct VFPContext));

#define RESTOREREGS(cc, sc)                                         		\
    CopyMemQuick(cc->regs.r, &GPSTATE(sc), sizeof(_STRUCT_ARM_THREAD_STATE));	\
    CopyMemQuick(cc->fpuContext, &FPSTATE(sc), sizeof(struct VFPContext));

/* Print signal context. Used in crash handler */
#define PRINT_SC(sc) \
    bug ("    R0=%08X  R1=%08X  R2 =%08X  R3=%08X\n" \
    	 "    R4=%08X  R5=%08X  R6 =%08X  R7=%08X\n" \
    	 "    R8=%08x  R9=%08X  R10=%08X  FP=%08X\n" \
    	 "    IP=%08x  SP=%08X  LR=%08X   PC=%08X\n" \
	    , R0(sc), R1(sc), R2(sc), R3(sc)	\
	    , R4(sc), R5(sc), R6(sc), R7(sc)	\
	    , R8(sc), R9(sc), R10(sc), R11(sc)	\
	    , R12(sc), SP(sc), LR(sc), PC(sc)	\
	)

#endif /* __AROS_EXEC_LIBRARY__ */

/* We emulate 6 exceptions of ARM CPU (all but softint) */
#define EXCEPTIONS_COUNT 6

struct AROSCPUContext
{
    struct ExceptionContext regs;
    int	errno_backup;
    struct AROSCPUContext *sc;
};

#define GET_PC(ctx) (APTR)ctx->regs.pc
#define SET_PC(ctx, pc) ctx->regs.pc = (ULONG)pc
