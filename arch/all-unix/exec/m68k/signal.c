/*
 *  linux/arch/m68k/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/*
 * Linux/m68k support by Hamish Macdonald
 *
 * 68060 fixes by Jesper Skov
 */

/*
 * ++roman (07/09/96): implemented signal stacks (specially for tosemu on
 * Atari :-) Current limitation: Only one sigstack can be active at one time.
 * If a second signal with SA_STACK set arrives while working on a sigstack,
 * SA_STACK is ignored. This behaviour avoids lots of trouble with nested
 * signal handlers!
 */

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>

#include <asm/setup.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/traps.h>

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);
/* !!! old functionheader:
 * asmlinkage int do_signal(unsigned long oldmask, struct pt_regs *regs);
 * new functionheader:
 */
asmlinkage int do_signal(unsigned long oldmask,
			 struct switch_stack * sw, 
			 struct pt_regs *regs);

static const int extra_sizes[16] = {
  0,
  -1, /* sizeof(((struct frame *)0)->un.fmt1), */
  sizeof(((struct frame *)0)->un.fmt2),
  sizeof(((struct frame *)0)->un.fmt3),
  sizeof(((struct frame *)0)->un.fmt4),
  -1, /* sizeof(((struct frame *)0)->un.fmt5), */
  -1, /* sizeof(((struct frame *)0)->un.fmt6), */
  sizeof(((struct frame *)0)->un.fmt7),
  -1, /* sizeof(((struct frame *)0)->un.fmt8), */
  sizeof(((struct frame *)0)->un.fmt9),
  sizeof(((struct frame *)0)->un.fmta),
  sizeof(((struct frame *)0)->un.fmtb),
  -1, /* sizeof(((struct frame *)0)->un.fmtc), */
  -1, /* sizeof(((struct frame *)0)->un.fmtd), */
  -1, /* sizeof(((struct frame *)0)->un.fmte), */
  -1, /* sizeof(((struct frame *)0)->un.fmtf), */
};

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */

/* !!! old function header: 
 * asmlinkage int do_sigsuspend(struct pt_regs *regs)
 *
 * new function header:
 */
asmlinkage int do_sigsuspend(struct switch_stack * sw, struct pt_regs *regs)
{
	unsigned long oldmask = current->blocked;
	unsigned long newmask = regs->d3;

	current->blocked = newmask & _BLOCKABLE;
	regs->d0 = -EINTR;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
/* !!! old call to do_signal 
 *		if (do_signal(oldmask, regs))
 * new call to do_signal
 */
		if (do_signal(oldmask, sw, regs))
/* !!! */
			return -EINTR;
	}
}

static unsigned char fpu_version = 0;	/* version number of fpu, set by setup_frame */

asmlinkage int do_sigreturn(unsigned long __unused)
{
	struct sigcontext_struct context;
	struct pt_regs *regs;
	struct switch_stack *sw;
	int fsize = 0;
	int formatvec = 0;
	unsigned long fp;
	unsigned long usp = rdusp();

#if 0
	printk("sys_sigreturn, usp=%08x\n", (unsigned) usp);
#endif

	/* get stack frame pointer */
	sw = (struct switch_stack *) &__unused;
	regs = (struct pt_regs *) (sw + 1);

	/* get previous context (including pointer to possible extra junk) */
        if (verify_area(VERIFY_READ, (void *)usp, sizeof(context)))
                goto badframe;

	memcpy_fromfs(&context,(void *)usp, sizeof(context));

	fp = usp + sizeof (context);

	/* restore signal mask */
	current->blocked = context.sc_mask & _BLOCKABLE;

	/* !!! restore registers passed in the extended sigcontext-structure */
	regs->d2=context.sc_ext_d2;
	regs->d3=context.sc_ext_d3;
	regs->d4=context.sc_ext_d4;
	regs->d5=context.sc_ext_d5;
	
	sw->d6=context.sc_ext_d6;
	sw->d7=context.sc_ext_d7;
	sw->a2=context.sc_ext_a2;
	sw->a3=context.sc_ext_a3;
	sw->a4=context.sc_ext_a4;
	sw->a5=context.sc_ext_a5;
	sw->a6=context.sc_ext_a6;
	/* !!!  */

	/* restore passed registers */
	regs->d0 = context.sc_d0;
	regs->d1 = context.sc_d1;
	regs->a0 = context.sc_a0;
	regs->a1 = context.sc_a1;
	regs->sr = (regs->sr & 0xff00) | (context.sc_sr & 0xff);
	regs->pc = context.sc_pc;
	regs->orig_d0 = -1;		/* disable syscall checks */
	wrusp(context.sc_usp);
	formatvec = context.sc_formatvec;
	regs->format = formatvec >> 12;
	regs->vector = formatvec & 0xfff;
	if ((!CPU_IS_060 && context.sc_fpstate[0]) || (CPU_IS_060 && context.sc_fpstate[2])){
	    /* Verify the frame format.  */
	    if (!CPU_IS_060 && (context.sc_fpstate[0] != fpu_version))
	      goto badframe;
	    if (boot_info.cputype & FPU_68881)
	      {
		if (context.sc_fpstate[1] != 0x18
		    && context.sc_fpstate[1] != 0xb4)
		  goto badframe;
	      }
	    else if (boot_info.cputype & FPU_68882)
	      {
		if (context.sc_fpstate[1] != 0x38
		    && context.sc_fpstate[1] != 0xd4)
		  goto badframe;
	      }
	    else if (boot_info.cputype & FPU_68040)
	      {
		if (!(context.sc_fpstate[1] == 0x00 ||
                      context.sc_fpstate[1] == 0x28 ||
                      context.sc_fpstate[1] == 0x60))
		  goto badframe;
	      }
	    else if (boot_info.cputype & FPU_68060)
	      {
		if (!(context.sc_fpstate[3] == 0x00 ||
                      context.sc_fpstate[3] == 0x60 ||
		      context.sc_fpstate[3] == 0xe0))
		  goto badframe;
	      }
	    __asm__ volatile ("fmovemx %0,%/fp0-%/fp1\n\t"
			      "fmoveml %1,%/fpcr/%/fpsr/%/fpiar"
			      : /* no outputs */
			      : "m" (*context.sc_fpregs),
				"m" (*context.sc_fpcntl));
	  }
	__asm__ volatile ("frestore %0" : : "m" (*context.sc_fpstate));

	fsize = extra_sizes[regs->format];
	if (fsize < 0) {
		/*
		 * user process trying to return with weird frame format
		 */
#if DEBUG
		printk("user process returning with weird frame format\n");
#endif
		goto badframe;
	}

	if (context.sc_usp != fp+fsize)
		current->flags &= ~PF_ONSIGSTK;
	
	/* OK.	Make room on the supervisor stack for the extra junk,
	 * if necessary.
	 */

	if (fsize) {
		if (verify_area(VERIFY_READ, (void *)fp, fsize))
                        goto badframe;

#define frame_offset (sizeof(struct pt_regs)+sizeof(struct switch_stack))
		__asm__ __volatile__
			("movel %0,%/a0\n\t"
			 "subl %1,%/a0\n\t"     /* make room on stack */
			 "movel %/a0,%/sp\n\t"  /* set stack pointer */
			 /* move switch_stack and pt_regs */
			 "1: movel %0@+,%/a0@+\n\t"
			 "   dbra %2,1b\n\t"
			 "lea %/sp@(%c3),%/a0\n\t" /* add offset of fmt stuff */
			 "lsrl  #2,%1\n\t"
			 "subql #1,%1\n\t"
			 "2: movesl %4@+,%2\n\t"
			 "   movel %2,%/a0@+\n\t"
			 "   dbra %1,2b\n\t"
			 "bral " SYMBOL_NAME_STR(ret_from_signal)
			 : /* no outputs, it doesn't ever return */
			 : "a" (sw), "d" (fsize), "d" (frame_offset/4-1),
			   "n" (frame_offset), "a" (fp)
			 : "a0");
#undef frame_offset
		goto badframe;
		/* NOTREACHED */
	}

	return regs->d0;
badframe:
        do_exit(SIGSEGV);
}

/*
 * Set up a signal frame...
 *
 * This routine is somewhat complicated by the fact that if the
 * kernel may be entered by an exception other than a system call;
 * e.g. a bus error or other "bad" exception.  If this is the case,
 * then *all* the context on the kernel stack frame must be saved.
 *
 * For a large number of exceptions, the stack frame format is the same
 * as that which will be created when the process traps back to the kernel
 * when finished executing the signal handler.	In this case, nothing
 * must be done.  This is exception frame format "0".  For exception frame
 * formats "2", "9", "A" and "B", the extra information on the frame must
 * be saved.  This information is saved on the user stack and restored
 * when the signal handler is returned.
 *
 * The format of the user stack when executing the signal handler is:
 *
 *     usp ->  RETADDR (points to code below)
 *	       signum  (parm #1)
 *	       sigcode (parm #2 ; vector number)
 *	       scp     (parm #3 ; sigcontext pointer, pointer to #1 below)
 *	       code1   (addaw #20,sp) ; pop parms and code off stack
 *	       code2   (moveq #119,d0; trap #0) ; sigreturn syscall
 *     #1|     oldmask
 *	 |     old usp
 *	 |     d0      (first saved reg)
 *	 |     d1
 *	 |     a0
 *	 |     a1
 *	 |     sr      (saved status register)
 *	 |     pc      (old pc; one to return to)
 *	 |     forvec  (format and vector word of old supervisor stack frame)
 *	 |     floating point context
 *
 * These are optionally followed by some extra stuff, depending on the
 * stack frame interrupted. This is 1 longword for format "2", 3
 * longwords for format "9", 6 longwords for format "A", and 21
 * longwords for format "B".
 */

#define UFRAME_SIZE(fs) (sizeof(struct sigcontext_struct)/4 + 6 + fs/4)

/*!!! old functionheader:
 * static void setup_frame (struct sigaction * sa, struct pt_regs *regs,
 *			   int signr, unsigned long oldmask)
 * new functionheader:
 */
static void setup_frame (struct sigaction * sa, struct pt_regs *regs,
			 int signr, unsigned long oldmask,
			 struct switch_stack * sw)
 {
	struct sigcontext_struct context;
	unsigned long *frame, *tframe;
	int fsize = extra_sizes[regs->format];

	if (fsize < 0) {
		printk ("setup_frame: Unknown frame format %#x\n",
			regs->format);
		do_exit(SIGSEGV);
	}

	frame = (unsigned long *)rdusp();
	if (!(current->flags & PF_ONSIGSTK) && (sa->sa_flags & SA_STACK)) {
		frame = (unsigned long *)sa->sa_restorer;
		current->flags |= PF_ONSIGSTK;
	}
	frame -= UFRAME_SIZE(fsize);

	if (verify_area(VERIFY_WRITE,frame,UFRAME_SIZE(fsize)*4))
		do_exit(SIGSEGV);
	if (fsize) {
		memcpy_tofs (frame + UFRAME_SIZE(0), regs + 1, fsize);
		regs->stkadj = fsize;
	}

/* set up the "normal" stack seen by the signal handler */
	tframe = frame;

	/* return address points to code on stack */
	put_user((ulong)(frame+4), tframe); tframe++;
	if (current->exec_domain && current->exec_domain->signal_invmap)
	    put_user(current->exec_domain->signal_invmap[signr], tframe);
	else
	    put_user(signr, tframe);
	tframe++;

	put_user(regs->vector, tframe); tframe++;
	/* "scp" parameter.  points to sigcontext */
	put_user((ulong)(frame+6), tframe); tframe++;

/* set up the return code... */
	put_user(0xdefc0014,tframe); tframe++; /* addaw #20,sp */
	put_user(0x70774e40,tframe); tframe++; /* moveq #119,d0; trap #0 */

/* Flush caches so the instructions will be correctly executed. (MA) */
	cache_push_v ((unsigned long)frame, (int)tframe - (int)frame);

	/* !!! setup and copy registers passed in the extended 
	 * sigcontext-structure 
	 */
	context.sc_ext_d2=regs->d2;
	context.sc_ext_d3=regs->d3;
	context.sc_ext_d4=regs->d4;
	context.sc_ext_d5=regs->d5;

	context.sc_ext_d6=sw->d6;
	context.sc_ext_d7=sw->d7;
	context.sc_ext_a2=sw->a2;
	context.sc_ext_a3=sw->a3;
	context.sc_ext_a4=sw->a4;
	context.sc_ext_a5=sw->a5;
	context.sc_ext_a6=sw->a6;
	/* !!!  */

/* setup and copy the sigcontext structure */
	context.sc_mask       = oldmask;
	context.sc_usp	      = rdusp();
	context.sc_d0	      = regs->d0;
	context.sc_d1	      = regs->d1;
	context.sc_a0	      = regs->a0;
	context.sc_a1	      = regs->a1;
	context.sc_sr	      = regs->sr;
	context.sc_pc	      = regs->pc;
	context.sc_formatvec  = (regs->format << 12 | regs->vector);
	__asm__ volatile ("fsave %0" : : "m" (*context.sc_fpstate) : "memory");
	if ((!CPU_IS_060 && context.sc_fpstate[0]) || (CPU_IS_060 && context.sc_fpstate[2])){
		fpu_version = context.sc_fpstate[0];
		__asm__ volatile ("fmovemx %/fp0-%/fp1,%0\n\t"
				  "fmoveml %/fpcr/%/fpsr/%/fpiar,%1"
				  : /* no outputs */
				  : "m" (*context.sc_fpregs),
				  "m" (*context.sc_fpcntl)
				  : "memory");
	}
	memcpy_tofs (tframe, &context, sizeof(context));

	/*
	 * no matter what frame format we were using before, we
	 * will do the "RTE" using a normal 4 word frame.
	 */
	regs->format = 0;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) sa->sa_handler;

	/* Prepare to skip over the extra stuff in the exception frame.  */
	if (regs->stkadj) {
		struct pt_regs *tregs =
			(struct pt_regs *)((ulong)regs + regs->stkadj);
#if DEBUG
		printk("Performing stackadjust=%04x\n", regs->stkadj);
#endif
		/* This must be copied with decreasing addresses to
                   handle overlaps.  */
		tregs->vector = regs->vector;
		tregs->format = regs->format;
		tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}
}

/*
 * OK, we're invoking a handler
 */	

/* !!! old function header:
 * static inline void handle_signal(unsigned long signr, struct sigaction *sa,
 *				    unsigned long oldmask, struct pt_regs *regs)
 * new function header:
 */
static void handle_signal(unsigned long signr, struct sigaction *sa,
			  unsigned long oldmask, struct pt_regs *regs,
			  struct switch_stack *sw)
{
	/* are we from a system call? */
	if (regs->orig_d0 >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->d0) {
			case -ERESTARTNOHAND:
				regs->d0 = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->d0 = -EINTR;
					break;
				}
			/* fallthrough */
			case -ERESTARTNOINTR:
				regs->d0 = regs->orig_d0;
				regs->pc -= 2;
		}
	}

	/* set up the stack frame */
	/* !!! old call of setup_frame: 
	 * setup_frame(sa, regs, signr, oldmask);
	 *
	 * new call of setup_frame:
	 */
	setup_frame(sa, regs, signr, oldmask, sw);
	/* !!! */

	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	if (!(sa->sa_flags & SA_NOMASK))
		current->blocked |= (sa->sa_mask | _S(signr)) & _BLOCKABLE;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals
 * that the kernel can handle, and then we build all the user-level signal
 * handling stack-frames in one go after that.
 */

/* !!! old functionheader: 
 * asmlinkage int do_signal(unsigned long oldmask, struct pt_regs *regs);
 *
 * new functionheader:
 */

asmlinkage int do_signal(unsigned long oldmask,
			 struct switch_stack *sw, 
			 struct pt_regs *regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long signr;
	struct sigaction * sa;

	current->tss.esp0 = (unsigned long) regs;

	/* If the process is traced, all signals are passed to the debugger. */
	if (current->flags & PF_PTRACED)
		mask = ~0UL;
	while ((signr = current->signal & mask)) {
		__asm__("bfffo  %2,#0,#0,%1\n\t"
			"bfclr  %0,%1,#1\n\t"
			"eorw   #31,%1"
			:"=m" (current->signal),"=d" (signr)
			:"0" (current->signal), "1" (signr));
		sa = current->sig->action + signr;
		signr++;

		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;

			/* Did we come from a system call? */
			if (regs->orig_d0 >= 0) {
				/* Restart the system call */
				if (regs->d0 == -ERESTARTNOHAND ||
				    regs->d0 == -ERESTARTSYS ||
				    regs->d0 == -ERESTARTNOINTR) {
					regs->d0 = regs->orig_d0;
					regs->pc -= 2;
				}
			}
			notify_parent(current);
			schedule();
			if (!(signr = current->exit_code)) {
			discard_frame:
			    /* Make sure that a faulted bus cycle
			       isn't restarted (only needed on the
			       68030).  */
			    if (regs->format == 10 || regs->format == 11) {
				regs->stkadj = extra_sizes[regs->format];
				regs->format = 0;
			    }
			    continue;
			}
			current->exit_code = 0;
			if (signr == SIGSTOP)
				goto discard_frame;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				mask &= ~_S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
			case SIGSTOP:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags & 
				      SA_NOCLDSTOP))
					notify_parent(current);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGIOT: case SIGFPE: case SIGSEGV:
				if (current->binfmt && current->binfmt->core_dump) {
				    if (current->binfmt->core_dump(signr, regs))
					signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		/* !!! old call of handle_signal:
	 	 * handle_signal(signr, sa, oldmask, regs);
	 	 *
	 	 * new call of handle_signal:
	 	 */
		handle_signal(signr, sa, oldmask, regs, sw);
		/* !!! */
		return 1;
	}

	/* Did we come from a system call? */
	if (regs->orig_d0 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->d0 == -ERESTARTNOHAND ||
		    regs->d0 == -ERESTARTSYS ||
		    regs->d0 == -ERESTARTNOINTR) {
			regs->d0 = regs->orig_d0;
			regs->pc -= 2;
		}
	}

	/* If we are about to discard some frame stuff we must copy
	   over the remaining frame. */
	if (regs->stkadj) {
		struct pt_regs *tregs =
		  (struct pt_regs *) ((ulong) regs + regs->stkadj);

		/* This must be copied with decreasing addresses to
		   handle overlaps.  */
		tregs->vector = regs->vector;
		tregs->format = regs->format;
		tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}
	return 0;
}
