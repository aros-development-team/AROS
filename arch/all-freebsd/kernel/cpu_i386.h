/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/i386/cpucontext.h>

#ifndef __AROS_EXEC_LIBRARY__

#include <machine/psl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ucontext.h>
#include <signal.h>

typedef struct sigcontext regs_t;
#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	__sighandler_t *

#define SC_DISABLE(sc)   (sc->sc_mask = KernelBase->kb_PlatformData->sig_int_mask)
#define SC_ENABLE(sc)    (pd->iface->SigEmptySet(&sc->sc_mask))

#define SP(sc)           (sc->sc_esp)
#define FP(sc)           (sc->sc_ebp)
#define PC(sc)           (sc->sc_eip)

#define R0(sc)           (sc->sc_eax)
#define R1(sc)           (sc->sc_ebx)
#define R2(sc)           (sc->sc_ecx)
#define R3(sc)           (sc->sc_edx)
#define R4(sc)           (sc->sc_edi)
#define R5(sc)           (sc->sc_esi)
#define R6(sc)           (sc->sc_efl)

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, int code, struct sigcontext *sc) \
	{						     			   \
	    sighandler( sig, (regs_t*)sc);             				   \
	}

/* Save and restore the CPU GPRs in the CPU context */
#define SAVE_CPU(cc, sc)	\
    cc.eax    = R0(sc);		\
    cc.ebx    = R1(sc);		\
    cc.ecx    = R2(sc);		\
    cc.edx    = R3(sc);		\
    cc.edi    = R4(sc);		\
    cc.esi    = R5(sc);		\
    cc.eflags = R6(sc);		\
    cc.ebp    = FP(sc);		\
    cc.eip    = PC(sc);		\
    cc.esp    = SP(sc);

#define RESTORE_CPU(cc, sc)								 \
    R0(sc) = cc.eax;									 \
    R1(sc) = cc.ebx;									 \
    R2(sc) = cc.ecx;    								 \
    R3(sc) = cc.edx;									 \
    R4(sc) = cc.edi;									 \
    R5(sc) = cc.esi;        								 \
    R6(sc) = sc->sc_efl = (sc->sc_efl & ~PSL_USERCHANGE) | (cc.eflags & PSL_USERCHANGE); \
    FP(sc) = cc.ebp;									 \
    PC(sc) = cc.eip;									 \
    SP(sc) = cc.esp;

#if __FreeBSD_version >= 500001
/*
 * This is the FreeBSD 5.x and higher version
 */
#define SAVE_FPU(cc, sc)                                \
{							\
    APTR dest;						\
    ULONG flag, len;					\
    (cc)->fpformat = (sc)->sc_fpformat;			\
    (cc)->ownedfp  = (sc)->sc_ownedfp;			\
    switch (cc)->ownedfp				\
    {							\
    case _MC_FPOWNED_PCB:				\
        flag = ECF_FPX; 				\
    	dest = (cc)->regs.FPXData;			\
    	len = sizeof(struct FPXContext);		\
    	break;						\
    case _MC_FPOWNED_FPU:				\
        flag = ECF_FPU;					\
    	dest = (cc)->regs.FPUData;			\
    	len  = sizeof(struct FPUContext);		\
    	break;						\
    default:						\
    	dest = NULL;					\
    	break;						\
    }							\
    if (dest)						\
    {							\
        (cc)->regs.Flags |= flag;			\
        CopyMemQuick(sc->sc_fpstate, dest, len);	\
    }							\
}

#define RESTORE_FPU(cc, sc)                             \
{							\
    APTR data;						\
    ULONG flag, len;					\
    if ((cc)->fpformat)					\
    {							\
	(sc)->sc_fpformat = (cc)->fpformat;	        \
	(sc)->sc_ownedfp  = (cc)->ownedfp;           	\
    }							\
    switch (sc)->sc_ownedfp				\
    {							\
    case _MC_FPOWNED_PCB:				\
        flag = ECF_FPX; 				\
    	data = (cc)->regs.FPXData;			\
    	len = sizeof(struct FPXContext);		\
    	break;						\
    case _MC_FPOWNED_FPU:				\
        flag = ECF_FPU;					\
    	data = (cc)->regs.FPUData;			\
    	len  = sizeof(struct FPUContext);		\
    	break;						\
    default:						\
    	data = NULL;					\
    	break;						\
    }							\
    if ((cc)->regs.Flags & flag)			\
        CopyMemQuick(data, sc->sc_fpstate, len);	\
}

#else

/*
 * FreeBSD 4 and below have a different context format.
 * Note that SSE is not supported at all. And on SSE machines
 * legacy FPU context will not be saved (we don't define USE_LEGACY_8087
 * in order to save some memory).
 * This should not really matter because FreeBSD v4 is ancient history.
 */
#define SAVE_FPU(cc, sc)                                              		\
    if ((cc)->FPUData)								\
    {										\
    	(cc)->Flags |= ECF_FPU;							\
    	CopyMemQuick(sc->sc_fpregs, (cc)->FPUData, sizeof(struct FPUContext));	\
    }

#define RESTORE_FPU(cc, sc)                                           		\
    if ((cc)->Flags & ECF_FPU)							\
    	CopyMemQuick((cc)->FPUData, sc->sc_fpregs, sizeof(struct FPUContext));

#endif

#define SAVEREGS(cc, sc)        	\
    SAVE_CPU((cc)->regs, sc);		\
    (cc)->isp = sc->sc_isp;		\
    SAVE_FPU((cc), sc);			\

#define RESTOREREGS(cc, sc)     	\
    RESTORE_CPU((cc)->regs, sc);	\
    sc->sc_isp = (cc)->isp;		\
    RESTORE_FPU((cc), sc);

#define PRINT_SC(sc)                                        \
    bug("    SP=%08lx  FP=%08lx  PC=%08lx\n"        	    \
	"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n"      \
	"    R4=%08lx  R5=%08lx  R6=%08lx\n"                \
	, SP(sc), FP(sc), PC(sc)                            \
	, R0(sc), R1(sc), R2(sc), R3(sc)                    \
	, R4(sc), R5(sc), R6(sc)                            \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 17

struct AROSCPUContext
{
    struct ExceptionContext regs; /* Public portion */
    ULONG isp;			  /* Host-specific stuff follows */
    int fpformat;
    int ownedfp;
    int	errno_backup;
};
