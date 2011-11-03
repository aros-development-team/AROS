/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros to handle unix signals, PPC version
    Lang: english
*/

#ifndef _SIGCORE_H
#define _SIGCORE_H

#include <exec/types.h>
#include <aros/ppc/cpucontext.h>

#ifdef __AROS_EXEC_LIBRARY__

struct ucontext;

#else

/* We don't use any hacks any more. With modern kernel and libc it's okay */
#define SIGCORE_NEED_SA_SIGINFO 1

#include <ucontext.h>
#include <signal.h>

#ifndef _SIGNAL_H
#define _SIGNAL_H
#endif
#ifndef __KERNEL_STRICT_NAMES
#define __KERNEL_STRICT_NAMES
#endif
#include <bits/sigcontext.h>

/* name and type of the signal handler */
#define SIGHANDLER	linux_sighandler
#define SIGHANDLER_T	__sighandler_t

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, siginfo_t *blub, struct ucontext *u) 	\
	{ 										\
	    sighandler(sig, u); 							\
	}

/* Macros to access the stack pointer and program counter,
   PC is the current address in the program code. */

#define SP(uc)	(uc->uc_mcontext.regs->gpr[1])
#define PC(uc)	(uc->uc_mcontext.regs->nip)

/* Macros to enable or disable all signals after the signal handler
   has returned and the normal execution commences.
   On PowerPC this is the same as on x86-64. */
#define SC_DISABLE(uc) uc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(uc)  pd->iface->SigEmptySet(&uc->uc_sigmask)

/*
 * This macro saves all registers. Use this macro when you want
 * to leave the current tasks' context.
 * It saves as much is possible and sets appropriate flags in
 * context structure.
 * TODO: check if Traptype usage is correct.
 */
#define SAVEREGS(cc, uc)           				\
    do                             				\
    {                              				\
	long i;							\
	(cc)->regs.Flags = ECF_FULL_GPRS|ECF_FPU;		\
	for (i = 0; i < 32; i++)				\
	    (cc)->regs.gpr[i] = uc->uc_mcontext.regs->gpr[i];	\
	(cc)->regs.ip 	= uc->uc_mcontext.regs->nip;		\
	(cc)->regs.msr	= uc->uc_mcontext.regs->msr;		\
	(cc)->regs.ctr	= uc->uc_mcontext.regs->ctr;		\
	(cc)->regs.lr 	= uc->uc_mcontext.regs->link;		\
	(cc)->regs.xer	= uc->uc_mcontext.regs->xer;		\
	(cc)->regs.cr 	= uc->uc_mcontext.regs->ccr;		\
	(cc)->regs.Traptype = uc->uc_mcontext.regs->trap;	\
	(cc)->regs.dar	= uc->uc_mcontext.regs->dar;		\
	(cc)->regs.dsisr = uc->uc_mcontext.regs->dsisr;		\
	(cc)->orig_gpr3	= uc->uc_mcontext.regs->orig_gpr3;	\
	(cc)->result	= uc->uc_mcontext.regs->result;		\
	for (i = 0; i < 32; i++)				\
	    (cc)->regs.fpr[i] = *(double*)&(uc->uc_mcontext.regs->gpr[PT_FPR0 + 2 * i]); \
    } while(0)

/*
 * This macro does the opposite to SAVEREGS(). It restores all
 * registers which are present in context (indicated by its flags).
 * After that, you can enter the new task's context.
 */
#define RESTOREREGS(cc, uc)           				\
    do								\
    {								\
    	long i;							\
	long n = ((cc)->regs.Flags & ECF_FULL_GPRS) ? 32 : 14;	\
    	for (i = 0; i < n; i++)					\
	    uc->uc_mcontext.regs->gpr[i] = (cc)->regs.gpr[i];	\
	uc->uc_mcontext.regs->nip	= (cc)->regs.ip;	\
	uc->uc_mcontext.regs->msr	= (cc)->regs.msr;	\
	uc->uc_mcontext.regs->ctr	= (cc)->regs.ctr;	\
	uc->uc_mcontext.regs->link	= (cc)->regs.lr;	\
	uc->uc_mcontext.regs->xer	= (cc)->regs.xer;	\
	uc->uc_mcontext.regs->ccr	= (cc)->regs.cr;	\
	uc->uc_mcontext.regs->trap	= (cc)->regs.Traptype;	\
	uc->uc_mcontext.regs->dar	= (cc)->regs.dar;	\
	uc->uc_mcontext.regs->dsisr	= (cc)->regs.dsisr;	\
	uc->uc_mcontext.regs->orig_gpr3 = (cc)->orig_gpr3;	\
	uc->uc_mcontext.regs->result    = (cc)->result;		\
	if ((cc)->regs.Flags & ECF_FPU)				\
	{							\
	    for (i = 0; i < 32; i++)				\
		*(double*)&(uc->uc_mcontext.regs->gpr[PT_FPR0 + 2 * i]) = (cc)->regs.fpr[i]; \
	}							\
    } while(0)

/* This macro prints the current signals' context */

#define PRINT_SC(sc) \
	bug ("SC: SP=%08lx  PC=%08lx\n" \
	    , SP(sc), PC(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

/*
 * We support 14 exceptions for PPC (see arch/all-unix/kernel/cpu_ppc.c).
 * Note that krnRunExceptionHandlers() does not check exception number,
 * so increase this if you use higher numbers.
 */
#define EXCEPTIONS_COUNT 14

typedef struct ucontext regs_t;

/* This structure is used to save/restore registers */
struct AROSCPUContext
{
    struct ExceptionContext regs;	  /* Public portion	 */
    unsigned long	    orig_gpr3;	  /* Host-specific magic */
    unsigned long	    result;
    int			    errno_backup;
};

#endif /* _SIGCORE_H */
