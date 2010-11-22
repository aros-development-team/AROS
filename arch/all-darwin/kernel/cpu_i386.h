/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/types.h>

#ifdef __AROS_EXEC_LIBRARY__

/*
 * We need these definitions here because struct AROSCPUContext below
 * is (wrongly) accessed from rom/exec, and we don't need generic AROS code
 * to depend on host OS includes.
 * In fact this is a hack. We urgently need to unify CPU context structure and
 * make it public. In this case exec will not need refer to this file any more.
 */

typedef struct 
{
    char mmst_reg[10];
    char mmst_rsrv[6];
} _STRUCT_MMST_REG;

typedef struct
{
    char xmm_reg[16];
} _STRUCT_XMM_REG;

typedef struct
{
    int 	     fpu_reserved[2];
    unsigned short   fpu_fcw;
    unsigned short   fpu_fsw;
    uint8_t	     fpu_ftw;
    uint8_t	     fpu_rsrv1;
    uint16_t	     fpu_fop;
    uint32_t	     fpu_ip;
    uint16_t	     fpu_cs;
    uint16_t	     fpu_rsrv2;
    uint32_t	     fpu_dp;
    uint16_t	     fpu_ds;
    uint16_t	     fpu_rsrv3;
    uint32_t	     fpu_mxcsr;
    uint32_t	     fpu_mxcsrmask;
    _STRUCT_MMST_REG fpu_stmm0;
    _STRUCT_MMST_REG fpu_stmm1;
    _STRUCT_MMST_REG fpu_stmm2;
    _STRUCT_MMST_REG fpu_stmm3;
    _STRUCT_MMST_REG fpu_stmm4;
    _STRUCT_MMST_REG fpu_stmm5;
    _STRUCT_MMST_REG fpu_stmm6;
    _STRUCT_MMST_REG fpu_stmm7;
    _STRUCT_XMM_REG  fpu_xmm0;
    _STRUCT_XMM_REG  fpu_xmm1;
    _STRUCT_XMM_REG  fpu_xmm2;
    _STRUCT_XMM_REG  fpu_xmm3;
    _STRUCT_XMM_REG  fpu_xmm4;
    _STRUCT_XMM_REG  fpu_xmm5;
    _STRUCT_XMM_REG  fpu_xmm6;
    _STRUCT_XMM_REG  fpu_xmm7;
    char	     fpu_rsrv4[14*16];
    int 	     fpu_reserved1;
} _STRUCT_X86_FLOAT_STATE32;

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
#define SC_ENABLE(sc)    KernelIFace.SigEmptySet(&(sc)->uc_sigmask)

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

#define FPSTATE(context) ((context)->uc_mcontext->__fs)

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

#define FPSTATE(context) ((context)->uc_mcontext->fs)

#endif 

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, ucontext_t *sc) \
	{						     \
	    sighandler(sig, sc);		             \
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
		(cc)->fpstate = FPSTATE(sc);				    \
		(cc)->have_fpu_data = 1;				    \
            } while (0)

#       define RESTORE_FPU(cc,sc)                                           \
            do {                                                            \
			  FPSTATE(sc) = (cc)->fpstate;									\
            } while (0)

#       define HAS_FPU(sc)      1

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
            SAVE_FPU(cc, sc);                                       \
        SAVE_CPU(cc, sc);                                           \
    } while (0)

#define RESTOREREGS(cc, sc)                                         \
    do {                                                            \
	RESTORE_CPU((cc),sc);                                       \
        if (HAS_FPU(sc) && (cc)->have_fpu_data)                     \
            RESTORE_FPU((cc),sc);                                   \
	} while (0)

#define PRINT_SC(sc) \
	bug ("    ESP=%08x  EBP=%08x  EIP=%08x  FPU=%s\n" \
		"    EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n" \
		"    EDI=%08x  ESI=%08x  EFLAGS=%08x\n" \
	    , SP(sc), FP(sc), PC(sc) \
	    , HAS_FPU(sc) ? "yes" : "no" \
	    , R0(sc), R1(sc), R2(sc), R3(sc) \
	    , R4(sc), R5(sc), R6(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 17

struct AROSCPUContext
{
    ULONG regs[9];	/* eax, ebx, ecx, edx, edi, esi, isp, fp, pc */
    int	errno_backup;
    _STRUCT_X86_FLOAT_STATE32 fpstate;
    int eflags;
    char have_fpu_data;
};

#define GET_PC(ctx) (APTR)ctx->regs[8]
#define SET_PC(ctx, pc) ctx->regs[8] = (ULONG)pc

/*
 * _STRUCT_X86_FLOAT_STATE32 is defined in such a way that aligning it is problematic
 * (it has 8 reserved bytes in the beginning). In order to work around this we just will not
 * restore FPU state from a newly allocated context. This is a temporary workaround until
 * CPU context format is unified accross all systems
 */
#define PREPARE_INITIAL_CONTEXT(cc) cc->have_fpu_data = 0;

#define PREPARE_INITIAL_FRAME(ctx, sp, startpc)     \
    do {                                            \
        ctx->regs[7] = 0;                           \
        ctx->regs[8] = (startpc);                   \
    } while (0)

#define PRINT_CPU_CONTEXT(ctx) \
	bug ("    EBP=%08x  EIP=%08x\n" \
	     "    EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n" \
	     "    EDI=%08x  ESI=%08x  EFLAGS=%08x\n" \
	    , ctx->regs[7], ctx->regs[8] \
	    , ctx->regs[0] \
	    , ctx->regs[1] \
	    , ctx->regs[2] \
	    , ctx->regs[3] \
	    , ctx->regs[4] \
	    , ctx->regs[5] \
	    , ctx->regs[6] \
	)
