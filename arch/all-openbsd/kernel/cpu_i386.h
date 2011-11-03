/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __AROS_EXEC_LIBRARY__

#include <signal.h>

typedef struct sigcontext regs_t;
#define SIGHANDLER	bsd_sighandler
#define SIGHANDLER_T	void *	

#define SC_DISABLE(sc)   (sc->sc_mask = KernelBase->kb_PlatformData->sig_int_mask)
#define SC_ENABLE(sc)    (pd->iface->SigEmptySet(&sc->sc_mask))

#define SP(sc)       (sc->sc_esp)
#define FP(sc)       (sc->sc_ebp)
#define PC(sc)       (sc->sc_eip)

#define R0(sc)           (sc->sc_eax)
#define R1(sc)           (sc->sc_ebx)
#define R2(sc)           (sc->sc_ecx)
#define R3(sc)           (sc->sc_edx)
#define R4(sc)           (sc->sc_edi)
#define R5(sc)           (sc->sc_esi)
#define R6(sc)           (sc->sc_eflags) 

#define GLOBAL_SIGNAL_INIT(sighandler)						   \
	static void sighandler ## _gate (int sig, int code, struct sigcontext *sc) \
	{						     			   \
	    sighandler( sig, (regs_t*)sc);       				   \
	}

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
    cc.esp    = SP(sc);		\

/*
 * Restore CPU registers.
 * Note that we do not restore segment registers because they
 * are of own use by the host OS.
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

/* TODO: FPU/SSE support */

#define SAVEREGS(ctx, sc)     		\
    SAVE_CPU((ctx)->regs, sc);

#define RESTOREREGS(ctx, sc)		\
    RESTORE_CPU((ctx)->regs, sc);

#define PRINT_SC(sc) \
    bug("    SP=%08lx  FP=%08lx  PC=%08lx\n" \
	"    R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n" \
	"    R4=%08lx  R5=%08lx  R6=%08lx\n" \
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
