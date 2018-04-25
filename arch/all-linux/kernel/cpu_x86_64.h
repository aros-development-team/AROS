#ifndef _SIGCORE_H
#define _SIGCORE_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Macros to handle unix signals, x86_64 version.
    Lang: english
*/

#include <aros/x86_64/cpucontext.h>

#ifdef __AROS_EXEC_LIBRARY__

struct ucontext;
typedef struct ucontext regs_t;

#else

/* There's no need for USE_SA_SIGINFO, as we always use SA_SIGINFO
   on x86_64 - there's no other sane way to get ucontext. */ 
#define SIGCORE_NEED_SA_SIGINFO 1

/* Make _GNU_SOURCE work */
#undef _FEATURES_H
/* We want to use some predefined register names */
#define _GNU_SOURCE

#include <ucontext.h>
#include <signal.h>
#include <stddef.h>
#include <strings.h>

#ifndef _SIGNAL_H
#define _SIGNAL_H
#endif
#ifndef __KERNEL_STRICT_NAMES
#define __KERNEL_STRICT_NAMES
#endif
#include <bits/sigcontext.h>

typedef ucontext_t regs_t;

/* name and type of the signal handler */
#define SIGHANDLER	linux_sighandler
#define SIGHANDLER_T	__sighandler_t

/*
    This macro contains some magic necessary to make it work.
    The problem is that Linux offers no official way to obtain the
    signals' context. Linux stores the signals' context on the
    process' stack. It looks like this:
    Attention: As of version 2.2 of the Linux kernel there is
               not enough room on the stack anymore to save
               any registers on it. So everything has to go into
               the context structure. Previously PC and FP used 
               to be saved on the stack but now this would over-
               write some return address.
               The stack you see below is the stack of the last 
               active task within AROS. The linux kernel puts
               all kinds of other junk on it.

		    |			       |
		    +--------------------------+
		    | last entry before signal |
		    +--------------------------+
		    |	   signal context      |
		    +--------------------------+
		    |	   signal number       |
		    +--------------------------+
		    |	   return address      | 
		    +--------------------------+
		    |			       |

    so the address of the signal context is &sig+1.

    Well, we use the SIGCORE_NEED_SA_SIGINFO approach, so part of
    the above is only kept for historical reasons.
*/

#define GLOBAL_SIGNAL_INIT(sighandler) \
	static void sighandler ## _gate (int sig, siginfo_t *blub, void *u) 	\
	{ 										\
	    sighandler(sig, u); 							\
	}

/*
    Macros to access the stack pointer, frame pointer and program
    counter. The FP is the base address for accesses to arguments
    and local variables of a function and PC is the current address
    in the program code.
*/

#define SP(uc)          ((uc)->uc_mcontext.gregs[REG_RSP])
#define FP(uc)          ((uc)->uc_mcontext.gregs[REG_RBP])
#define PC(uc)          ((uc)->uc_mcontext.gregs[REG_RIP])

/*
    Macros to enable or disable all signals after the signal handler
    has returned and the normal execution commences.

    On Linux x86-64 signal mask is restored from uc_sigmask field of ucontext
    structure.
*/
#define SC_DISABLE(uc) uc->uc_sigmask = KernelBase->kb_PlatformData->sig_int_mask
#define SC_ENABLE(uc)  pd->iface->SigEmptySet(&uc->uc_sigmask)

/*
    The names of the general purpose registers which are to be saved.
    Use R and a number as name, no matter what the real name is.
    General purpose registers (GPRs) are registers which can be
    modified by the task (ie. data and address registers) and which are
    not saved by the CPU when an interrupt happens.
*/
#define R0(uc)          ((uc)->uc_mcontext.gregs[REG_RAX])
#define R1(uc)          ((uc)->uc_mcontext.gregs[REG_RBX])
#define R2(uc)          ((uc)->uc_mcontext.gregs[REG_RCX])
#define R3(uc)          ((uc)->uc_mcontext.gregs[REG_RDX])
#define R4(uc)          ((uc)->uc_mcontext.gregs[REG_RDI])
#define R5(uc)          ((uc)->uc_mcontext.gregs[REG_RSI])
#define R6(uc)          ((uc)->uc_mcontext.gregs[REG_EFL])
#define R8(uc)          ((uc)->uc_mcontext.gregs[REG_R8])
#define R9(uc)          ((uc)->uc_mcontext.gregs[REG_R9])
#define R10(uc)         ((uc)->uc_mcontext.gregs[REG_R10])
#define R11(uc)         ((uc)->uc_mcontext.gregs[REG_R11])
#define R12(uc)         ((uc)->uc_mcontext.gregs[REG_R12])
#define R13(uc)         ((uc)->uc_mcontext.gregs[REG_R13])
#define R14(uc)         ((uc)->uc_mcontext.gregs[REG_R14])
#define R15(uc)         ((uc)->uc_mcontext.gregs[REG_R15])

/*
    Save and restore the CPU GPRs in the CPU context
*/
#define SAVE_CPU(cc, sc)	\
    cc.rax    = R0(sc);		\
    cc.rbx    = R1(sc);		\
    cc.rcx    = R2(sc);		\
    cc.rdx    = R3(sc);		\
    cc.rdi    = R4(sc);		\
    cc.rsi    = R5(sc);		\
    cc.rflags = R6(sc);		\
    cc.r8     = R8(sc);		\
    cc.r9     = R9(sc);		\
    cc.r10    = R10(sc);	\
    cc.r11    = R11(sc);	\
    cc.r12    = R12(sc);	\
    cc.r13    = R13(sc);	\
    cc.r14    = R14(sc);	\
    cc.r15    = R15(sc);	\
    cc.rbp    = FP(sc);		\
    cc.rip    = PC(sc);		\
    cc.rsp    = SP(sc);
         
/*
 * Restore CPU registers.
 * Note that we do not restore segment registers because they
 * are of own use by Linux.
 */
#define RESTORE_CPU(cc, sc) \
    R0(sc)  = cc.rax;       \
    R1(sc)  = cc.rbx;       \
    R2(sc)  = cc.rcx;       \
    R3(sc)  = cc.rdx;       \
    R4(sc)  = cc.rdi;       \
    R5(sc)  = cc.rsi;       \
    R6(sc)  = cc.rflags;    \
    R8(sc)  = cc.r8;	    \
    R9(sc)  = cc.r9;	    \
    R10(sc) = cc.r10;	    \
    R11(sc) = cc.r11;	    \
    R12(sc) = cc.r12;	    \
    R13(sc) = cc.r13;	    \
    R14(sc) = cc.r14;	    \
    R15(sc) = cc.r15;	    \
    FP(sc)  = cc.rbp;       \
    PC(sc)  = cc.rip;       \
    SP(sc)  = cc.rsp;

/*
 * Save all registers from UNIX signal context to AROS context.
 * Save also SSE state if the context has buffer. ECF_FPX will be set
 * if SSE state was copied.
 */
#define SAVEREGS(cc, sc)                                       					\
    SAVE_CPU((cc)->regs, sc);									\
    if (sc->uc_mcontext.fpregs && (cc)->regs.FXData)						\
    {												\
    	(cc)->regs.Flags |= ECF_FPX;								\
    	CopyMemQuick(sc->uc_mcontext.fpregs, (cc)->regs.FXData, sizeof(struct FPXContext));	\
    }

/*
 * Restore all registers from AROS context to UNIX signal context.
 * Check context flags to decide whether to restore SSE or not.
 */
#define RESTOREREGS(cc, sc)                                    					\
    RESTORE_CPU((cc)->regs, sc);								\
    if ((cc)->regs.Flags & ECF_FPX)								\
	CopyMemQuick((cc)->regs.FXData, sc->uc_mcontext.fpregs, sizeof(struct FPXContext));

/* Print signal context. Used in crash handler. */
#define PRINT_SC(sc)						\
    bug ("    RSP=%016lx  RBP=%016lx  RIP=%016lx\n"		\
	 "    RAX=%016lx  RBX=%016lx  RCX=%016lx  RDX=%016lx\n" \
	 "    RDI=%016lx  RSI=%016lx  RFLAGS=%016lx\n"		\
	 "    R8 =%016lx  R9 =%016lx  R10=%016lx  R11=%016lx\n" \
	 "    R12=%016lx  R13=%016lx  R14=%016lx  R15=%016lx\n" \
	 , SP(sc), FP(sc), PC(sc)				\
	 , R0(sc), R1(sc), R2(sc), R3(sc)			\
	 , R4(sc), R5(sc), R6(sc), R8(sc), R9(sc)		\
	 , R10(sc), R11(sc), R12(sc), R13(sc), R14(sc), R15(sc) \
	)

#endif /* __AROS_EXEC_LIBRARY__ */

#define EXCEPTIONS_COUNT 17


/* Use this structure to save/restore registers */
struct AROSCPUContext
{
    struct ExceptionContext regs;
    int	errno_backup;
};

#endif /* _SIGCORE_H */
