/* -*- mode: asm -*-
 *
 *  linux/arch/m68k/kernel/entry.S
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 *
 * Linux/m68k support by Hamish Macdonald
 *
 * 68060 fixes by Jesper Skov
 *
 */

/*
 * entry.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 */

/*
 * 12/03/96 Jes: Currently we only support m68k single-cpu systems, so
 *               all pointers that used to be 'current' are now entry
 *               number 0 in the 'current_set' list.
 */

#include <linux/sys.h>
#include <linux/config.h>
#include <linux/linkage.h>
#include <asm/entry.h>
#include <asm/setup.h>
#include <asm/segment.h>

.globl SYMBOL_NAME(system_call), SYMBOL_NAME(buserr), SYMBOL_NAME(trap)
.globl SYMBOL_NAME(resume), SYMBOL_NAME(ret_from_exception)
.globl SYMBOL_NAME(ret_from_signal)
.globl SYMBOL_NAME(inthandler), SYMBOL_NAME(sys_call_table)
.globl SYMBOL_NAME(sys_fork), SYMBOL_NAME(sys_clone)
.globl SYMBOL_NAME(ret_from_interrupt), SYMBOL_NAME(bad_interrupt)

.text
ENTRY(buserr)
	SAVE_ALL_INT
	GET_CURRENT(%d0)
	movel	%sp,%sp@- 		| stack frame pointer argument
	bsrl	SYMBOL_NAME(buserr_c)
	addql	#4,%sp
	jra	SYMBOL_NAME(ret_from_exception)

ENTRY(trap)
	SAVE_ALL_INT
	GET_CURRENT(%d0)
	movel	%sp,%sp@- 		| stack frame pointer argument
	bsrl	SYMBOL_NAME(trap_c)
	addql	#4,%sp
	jra	SYMBOL_NAME(ret_from_exception)

ENTRY(reschedule)
	| save top of frame
	pea	%sp@
	jbsr	SYMBOL_NAME(set_esp0)
	addql	#4,%sp

	pea	SYMBOL_NAME(ret_from_exception)
	jmp	SYMBOL_NAME(schedule)

badsys:
	movel	#-LENOSYS,LPT_OFF_D0(%sp)
	jra	SYMBOL_NAME(ret_from_exception)

do_trace:
	movel	#-LENOSYS,LPT_OFF_D0(%sp)	| needed for strace
	subql	#4,%sp
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(syscall_trace)
	RESTORE_SWITCH_STACK
	addql	#4,%sp
	jbsr	@(SYMBOL_NAME(sys_call_table),%d2:l:4)@(0)
	movel	%d0,%sp@(LPT_OFF_D0)	| save the return value
	subql	#4,%sp			| dummy return address
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(syscall_trace)

SYMBOL_NAME_LABEL(ret_from_signal)
	RESTORE_SWITCH_STACK
	addql	#4,%sp
	jra	SYMBOL_NAME(ret_from_exception)

ENTRY(system_call)
	SAVE_ALL_SYS
	movel	%d0,%d2

	GET_CURRENT(%d0)
	| save top of frame
	pea	%sp@
	jbsr	SYMBOL_NAME(set_esp0)
	addql	#4,%sp

	cmpl	#NR_syscalls,%d2
	jcc	badsys
	btst	#LPF_TRACESYS_BIT,%curptr@(LTASK_FLAGS+LPF_TRACESYS_OFF)
	jne	do_trace
	jbsr	@(SYMBOL_NAME(sys_call_table),%d2:l:4)@(0)
	movel	%d0,%sp@(LPT_OFF_D0)	| save the return value

SYMBOL_NAME_LABEL(ret_from_exception)
	btst	#5,%sp@(LPT_OFF_SR)	| check if returning to kernel
	bnes	2f			| if so, skip resched, signals
	tstl	SYMBOL_NAME(need_resched)
	jne	SYMBOL_NAME(reschedule)
	cmpl	#SYMBOL_NAME(task),%curptr	| task[0] cannot have signals
	jeq	2f
					| check for delayed trace
	bclr	#LPF_DTRACE_BIT,%curptr@(LTASK_FLAGS+LPF_DTRACE_OFF)
	jne	do_delayed_trace
5:
	tstl	%curptr@(LTASK_STATE)	| state
	jne	SYMBOL_NAME(reschedule)
	tstl	%curptr@(LTASK_COUNTER)	| counter
	jeq	SYMBOL_NAME(reschedule)

	movel	%curptr@(LTASK_BLOCKED),%d0
	movel	%d0,%d1			| save blocked in d1 for sig handling
	notl	%d0
	btst	#LPF_PTRACED_BIT,%curptr@(LTASK_FLAGS+LPF_PTRACED_OFF)
	jeq	1f
	moveq	#-1,%d0			| let the debugger see all signals
1:	andl	%curptr@(LTASK_SIGNAL),%d0
	jne	Lsignal_return
2:	RESTORE_ALL

Lsignal_return:
	subql	#4,%sp			| dummy return address
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
  | !!! also put a pointer to the switchstack structure onto the stack
	move.l	%sp@(4)
  | !!! 
	movel	%d1,%sp@-
	bsrl	SYMBOL_NAME(do_signal)
  | !!! adjust the stack after coming back
	addql	#4,%sp
  | !!!
	addql	#8,%sp
	RESTORE_SWITCH_STACK
	addql	#4,%sp
	RESTORE_ALL

do_delayed_trace:
	bclr	#7,%sp@(LPT_OFF_SR)	| clear trace bit in SR
	pea	1			| send SIGTRAP
	movel	%curptr,%sp@-
	pea	LSIGTRAP
	jbsr	SYMBOL_NAME(send_sig)
	addql	#8,%sp
	addql	#4,%sp
	jra	5b

/*
** This is the main interrupt handler, responsible for calling process_int()
*/
SYMBOL_NAME_LABEL(inthandler)
	SAVE_ALL_INT
	GET_CURRENT(%d0)
	addql	#1,SYMBOL_NAME(local_irq_count)
					|  put exception # in d0
	bfextu %sp@(LPT_OFF_FORMATVEC){#4,#10},%d0

	movel	%sp,%sp@-
	movel	%d0,%sp@- 		|  put vector # on stack
	jbsr	SYMBOL_NAME(process_int)|  process the IRQ
	addql	#8,%sp			|  pop parameters off stack

SYMBOL_NAME_LABEL(ret_from_interrupt)
	subql	#1,SYMBOL_NAME(local_irq_count)
	jeq	1f
2:	
	RESTORE_ALL
1:
#if 1
	bfextu  %sp@(LPT_OFF_SR){#5,#3},%d0	| Check for nested interrupt.
#if MAX_NOINT_IPL > 0
	cmpiw	#MAX_NOINT_IPL,%d0
#endif
	jhi	2b
#endif
	/* Let the rest run with interrupts allowed.  This is safe since
	   the kernel never uses a non-standard ipl and this is the outer
	   level interrupt. */
	andw	#ALLOWINT,%sr

	/* check if we need to do software interrupts */
	
	movel	SYMBOL_NAME(bh_active),%d0
	andl	SYMBOL_NAME(bh_mask),%d0
	jeq	SYMBOL_NAME(ret_from_exception)

	pea	SYMBOL_NAME(ret_from_exception)
	jra	SYMBOL_NAME(do_bottom_half)


/* Handler for uninitialized and spurious interrupts */

SYMBOL_NAME_LABEL(bad_interrupt)
	addql	#1,SYMBOL_NAME(num_spurious)
	rte

ENTRY(sys_fork)
	SAVE_SWITCH_STACK	
	pea	%sp@(SWITCH_STACK_SIZE)
	jbsr	SYMBOL_NAME(m68k_fork)
	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_clone)
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
	jbsr	SYMBOL_NAME(m68k_clone)
	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_sigsuspend)
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
  | !!! also put a pointer to the switchstack structure onto the stack
	pea	%sp@(4)
  | !!! 
	jbsr	SYMBOL_NAME(do_sigsuspend)
  | !!! adjust the stack after coming back
	addql	#4,%sp
  | !!!
 	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_sigreturn)
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(do_sigreturn)
	RESTORE_SWITCH_STACK
	rts

SYMBOL_NAME_LABEL(resume)
	/*
	 * Beware - when entering resume, offset of tss is in d1,
	 * prev (the current task) is in a0, next (the new task)
	 * is in a1 and d2.b is non-zero if the mm structure is
	 * shared between the tasks, so don't change these
	 * registers until their contents are no longer needed.
	 */

	/* offset of tss struct (processor state) from beginning
	   of task struct */
	addl	%d1,%a0

	/* save sr */
	movew	%sr,%a0@(LTSS_SR)

#if 0
	/* disable interrupts */
	oriw	#0x0700,%sr
#endif

	/* save fs (sfc,%dfc) (may be pointing to kernel memory) */
	movec	%sfc,%d0
	movew	%d0,%a0@(LTSS_FS)

	/* save usp */
	/* it is better to use a movel here instead of a movew 8*) */
	movec	%usp,%d0
	movel	%d0,%a0@(LTSS_USP)

	/* save non-scratch registers on stack */
	SAVE_SWITCH_STACK

	/* save current kernel stack pointer */
	movel	%sp,%a0@(LTSS_KSP)

	/* save floating point context */
	fsave	%a0@(LTSS_FPCTXT+27*4)

#if defined(CONFIG_M68060)
#if !defined(CPU_M68060_ONLY)
	btst	#3,SYMBOL_NAME(m68k_cputype)+3
	beqs	1f
#endif
	/* The 060 FPU keeps status in bits 15-8 of the first longword */
	tstb	%a0@(LTSS_FPCTXT+27*4+2)
	jeq	3f
#if !defined(CPU_M68060_ONLY)
	jra	2f
#endif
#endif /* CONFIG_M68060 */
#if !defined(CPU_M68060_ONLY)
1:	tstb	%a0@(LTSS_FPCTXT+27*4)
	jeq	3f
#endif
2:	fmovemx	%fp0-%fp7,%a0@(LTSS_FPCTXT)
	fmoveml	%fpcr/%fpsr/%fpiar,%a0@(LTSS_FPCTXT+24*4)
3:

	/* get pointer to tss struct (a1 contains new task) */
	movel	%a1,%curptr
	addl	%d1,%a1

	/* Skip address space switching if they are the same. */
	tstb	%d2
	jne	4f

#if defined(CPU_M68020_OR_M68030) && defined(CPU_M68040_OR_M68060)
	/* 68040 or 68060 ? */
	tstl	SYMBOL_NAME(m68k_is040or060)
	bnes	1f
#endif
#if defined(CPU_M68020_OR_M68030)
	/*
	 * switch address space
	 */

	/* flush MC68030/MC68020 caches (they are virtually addressed) */
	movec	%cacr,%d0
	oriw	#LFLUSH_I_AND_D,%d0
	movec	%d0,%cacr

	/* switch the root pointer */
	pmove	%a1@(LTSS_CRP),%crp
#endif

#if defined(CPU_M68020_OR_M68030) && defined(CPU_M68040_OR_M68060)
	jra	2f	/* skip m68040 stuff */
1:
#endif
#if defined(CPU_M68040_OR_M68060)
	/*
	 * switch address space
	 */
	.chip	68040

	/* flush address translation cache (user entries) */
	pflushan

	/* switch the root pointer */
	movel	%a1@(LTSS_CRP+4),%d0
	movec	%d0,%urp

#if defined (CONFIG_M68060)
	/* is it a '060 ? */
#if !defined(CPU_M68060_ONLY)
	btst	#3,SYMBOL_NAME(m68k_cputype)+3
	beqs	2f
#endif
	/* clear user entries in the branch cache */
	movec	%cacr,%d0
	orl	#0x00200000,%d0
	movec	%d0,%cacr
#endif /* CONFIG_M68060 */
	.chip	68k
#endif /* CPU_M68040_OR_M68060 */
2:
4:
	/* restore floating point context */

#if defined(CONFIG_M68060)
#if !defined(CPU_M68060_ONLY)
	btst	#3,SYMBOL_NAME(m68k_cputype)+3
	beqs	1f
#endif
	/* The 060 FPU keeps status in bits 15-8 of the first longword */
	tstb	%a1@(LTSS_FPCTXT+27*4+2)
	jeq	3f
#if !defined(CPU_M68060_ONLY)
	jra	2f
#endif
#endif /* CONFIG_M68060 */
#if !defined(CPU_M68060_ONLY)
1:	tstb	%a1@(LTSS_FPCTXT+27*4)
	jeq	3f
#endif	
2:	fmovemx	%a1@(LTSS_FPCTXT),%fp0-%fp7
	fmoveml	%a1@(LTSS_FPCTXT+24*4),%fpcr/%fpsr/%fpiar
3:	frestore %a1@(LTSS_FPCTXT+27*4)

	/* restore the kernel stack pointer */
	movel	%a1@(LTSS_KSP),%sp

	/* restore non-scratch registers */
	RESTORE_SWITCH_STACK

	/* restore user stack pointer */
	movel	%a1@(LTSS_USP),%a0
	movel	%a0,%usp

	/* restore fs (sfc,%dfc) */
	movew	%a1@(LTSS_FS),%a0
	movec	%a0,%sfc
	movec	%a0,%dfc

	/* restore status register */
	movew	%a1@(LTSS_SR),%sr

	rts

.data
ALIGN
SYMBOL_NAME_LABEL(sys_call_table)
	.long SYMBOL_NAME(sys_setup)		/* 0 */
	.long SYMBOL_NAME(sys_exit)
	.long SYMBOL_NAME(sys_fork)
	.long SYMBOL_NAME(sys_read)
	.long SYMBOL_NAME(sys_write)
	.long SYMBOL_NAME(sys_open)		/* 5 */
	.long SYMBOL_NAME(sys_close)
	.long SYMBOL_NAME(sys_waitpid)
	.long SYMBOL_NAME(sys_creat)
	.long SYMBOL_NAME(sys_link)
	.long SYMBOL_NAME(sys_unlink)		/* 10 */
	.long SYMBOL_NAME(sys_execve)
	.long SYMBOL_NAME(sys_chdir)
	.long SYMBOL_NAME(sys_time)
	.long SYMBOL_NAME(sys_mknod)
	.long SYMBOL_NAME(sys_chmod)		/* 15 */
	.long SYMBOL_NAME(sys_chown)
	.long SYMBOL_NAME(sys_break)
	.long SYMBOL_NAME(sys_stat)
	.long SYMBOL_NAME(sys_lseek)
	.long SYMBOL_NAME(sys_getpid)		/* 20 */
	.long SYMBOL_NAME(sys_mount)
	.long SYMBOL_NAME(sys_umount)
	.long SYMBOL_NAME(sys_setuid)
	.long SYMBOL_NAME(sys_getuid)
	.long SYMBOL_NAME(sys_stime)		/* 25 */
	.long SYMBOL_NAME(sys_ptrace)
	.long SYMBOL_NAME(sys_alarm)
	.long SYMBOL_NAME(sys_fstat)
	.long SYMBOL_NAME(sys_pause)
	.long SYMBOL_NAME(sys_utime)		/* 30 */
	.long SYMBOL_NAME(sys_stty)
	.long SYMBOL_NAME(sys_gtty)
	.long SYMBOL_NAME(sys_access)
	.long SYMBOL_NAME(sys_nice)
	.long SYMBOL_NAME(sys_ftime)		/* 35 */
	.long SYMBOL_NAME(sys_sync)
	.long SYMBOL_NAME(sys_kill)
	.long SYMBOL_NAME(sys_rename)
	.long SYMBOL_NAME(sys_mkdir)
	.long SYMBOL_NAME(sys_rmdir)		/* 40 */
	.long SYMBOL_NAME(sys_dup)
	.long SYMBOL_NAME(sys_pipe)
	.long SYMBOL_NAME(sys_times)
	.long SYMBOL_NAME(sys_prof)
	.long SYMBOL_NAME(sys_brk)		/* 45 */
	.long SYMBOL_NAME(sys_setgid)
	.long SYMBOL_NAME(sys_getgid)
	.long SYMBOL_NAME(sys_signal)
	.long SYMBOL_NAME(sys_geteuid)
	.long SYMBOL_NAME(sys_getegid)		/* 50 */
	.long SYMBOL_NAME(sys_acct)
	.long SYMBOL_NAME(sys_phys)
	.long SYMBOL_NAME(sys_lock)
	.long SYMBOL_NAME(sys_ioctl)
	.long SYMBOL_NAME(sys_fcntl)		/* 55 */
	.long SYMBOL_NAME(sys_mpx)
	.long SYMBOL_NAME(sys_setpgid)
	.long SYMBOL_NAME(sys_ulimit)
	.long SYMBOL_NAME(sys_olduname)
	.long SYMBOL_NAME(sys_umask)		/* 60 */
	.long SYMBOL_NAME(sys_chroot)
	.long SYMBOL_NAME(sys_ustat)
	.long SYMBOL_NAME(sys_dup2)
	.long SYMBOL_NAME(sys_getppid)
	.long SYMBOL_NAME(sys_getpgrp)		/* 65 */
	.long SYMBOL_NAME(sys_setsid)
	.long SYMBOL_NAME(sys_sigaction)
	.long SYMBOL_NAME(sys_sgetmask)
	.long SYMBOL_NAME(sys_ssetmask)
	.long SYMBOL_NAME(sys_setreuid)		/* 70 */
	.long SYMBOL_NAME(sys_setregid)
	.long SYMBOL_NAME(sys_sigsuspend)
	.long SYMBOL_NAME(sys_sigpending)
	.long SYMBOL_NAME(sys_sethostname)
	.long SYMBOL_NAME(sys_setrlimit)	/* 75 */
	.long SYMBOL_NAME(sys_getrlimit)
	.long SYMBOL_NAME(sys_getrusage)
	.long SYMBOL_NAME(sys_gettimeofday)
	.long SYMBOL_NAME(sys_settimeofday)
	.long SYMBOL_NAME(sys_getgroups)	/* 80 */
	.long SYMBOL_NAME(sys_setgroups)
	.long SYMBOL_NAME(old_select)
	.long SYMBOL_NAME(sys_symlink)
	.long SYMBOL_NAME(sys_lstat)
	.long SYMBOL_NAME(sys_readlink)		/* 85 */
	.long SYMBOL_NAME(sys_uselib)
	.long SYMBOL_NAME(sys_swapon)
	.long SYMBOL_NAME(sys_reboot)
	.long SYMBOL_NAME(old_readdir)
	.long SYMBOL_NAME(old_mmap)		/* 90 */
	.long SYMBOL_NAME(sys_munmap)
	.long SYMBOL_NAME(sys_truncate)
	.long SYMBOL_NAME(sys_ftruncate)
	.long SYMBOL_NAME(sys_fchmod)
	.long SYMBOL_NAME(sys_fchown)		/* 95 */
	.long SYMBOL_NAME(sys_getpriority)
	.long SYMBOL_NAME(sys_setpriority)
	.long SYMBOL_NAME(sys_profil)
	.long SYMBOL_NAME(sys_statfs)
	.long SYMBOL_NAME(sys_fstatfs)		/* 100 */
	.long SYMBOL_NAME(sys_ioperm)
	.long SYMBOL_NAME(sys_socketcall)
	.long SYMBOL_NAME(sys_syslog)
	.long SYMBOL_NAME(sys_setitimer)
	.long SYMBOL_NAME(sys_getitimer)	/* 105 */
	.long SYMBOL_NAME(sys_newstat)
	.long SYMBOL_NAME(sys_newlstat)
	.long SYMBOL_NAME(sys_newfstat)
	.long SYMBOL_NAME(sys_uname)
	.long SYMBOL_NAME(sys_ni_syscall)	/* iopl for i386 */ /* 110 */
	.long SYMBOL_NAME(sys_vhangup)
	.long SYMBOL_NAME(sys_idle)
	.long SYMBOL_NAME(sys_ni_syscall)	/* vm86old for i386 */
	.long SYMBOL_NAME(sys_wait4)
	.long SYMBOL_NAME(sys_swapoff)		/* 115 */
	.long SYMBOL_NAME(sys_sysinfo)
	.long SYMBOL_NAME(sys_ipc)
	.long SYMBOL_NAME(sys_fsync)
	.long SYMBOL_NAME(sys_sigreturn)
	.long SYMBOL_NAME(sys_clone)		/* 120 */
	.long SYMBOL_NAME(sys_setdomainname)
	.long SYMBOL_NAME(sys_newuname)
	.long SYMBOL_NAME(sys_cacheflush)	/* modify_ldt for i386 */
	.long SYMBOL_NAME(sys_adjtimex)
	.long SYMBOL_NAME(sys_mprotect)		/* 125 */
	.long SYMBOL_NAME(sys_sigprocmask)
	.long SYMBOL_NAME(sys_create_module)
	.long SYMBOL_NAME(sys_init_module)
	.long SYMBOL_NAME(sys_delete_module)
	.long SYMBOL_NAME(sys_get_kernel_syms)	/* 130 */
	.long SYMBOL_NAME(sys_quotactl)
	.long SYMBOL_NAME(sys_getpgid)
	.long SYMBOL_NAME(sys_fchdir)
	.long SYMBOL_NAME(sys_bdflush)
	.long SYMBOL_NAME(sys_sysfs)		/* 135 */
	.long SYMBOL_NAME(sys_personality)
	.long SYMBOL_NAME(sys_ni_syscall)	/* for afs_syscall */
	.long SYMBOL_NAME(sys_setfsuid)
	.long SYMBOL_NAME(sys_setfsgid)
	.long SYMBOL_NAME(sys_llseek)		/* 140 */
	.long SYMBOL_NAME(sys_getdents)
	.long SYMBOL_NAME(sys_select)
	.long SYMBOL_NAME(sys_flock)
	.long SYMBOL_NAME(sys_msync)
	.long SYMBOL_NAME(sys_readv)		/* 145 */
	.long SYMBOL_NAME(sys_writev)
	.long SYMBOL_NAME(sys_getsid)
	.long SYMBOL_NAME(sys_fdatasync)
	.long SYMBOL_NAME(sys_sysctl)
	.long SYMBOL_NAME(sys_mlock)		/* 150 */
	.long SYMBOL_NAME(sys_munlock)
	.long SYMBOL_NAME(sys_mlockall)
	.long SYMBOL_NAME(sys_munlockall)
	.long SYMBOL_NAME(sys_sched_setparam)
	.long SYMBOL_NAME(sys_sched_getparam)   /* 155 */
	.long SYMBOL_NAME(sys_sched_setscheduler)
	.long SYMBOL_NAME(sys_sched_getscheduler)
	.long SYMBOL_NAME(sys_sched_yield)
	.long SYMBOL_NAME(sys_sched_get_priority_max)
	.long SYMBOL_NAME(sys_sched_get_priority_min)  /* 160 */
	.long SYMBOL_NAME(sys_sched_rr_get_interval)
	.long SYMBOL_NAME(sys_nanosleep)
	.long SYMBOL_NAME(sys_mremap)
	.long SYMBOL_NAME(sys_setresuid)
	.long SYMBOL_NAME(sys_getresuid)	/* 165 */
	.long SYMBOL_NAME(sys_ni_syscall)	/* for vm86 */
	.long SYMBOL_NAME(sys_query_module)
	.long SYMBOL_NAME(sys_poll)
	.long SYMBOL_NAME(sys_nfsservctl)
	.rept NR_syscalls-(.-SYMBOL_NAME(sys_call_table))/4
		.long SYMBOL_NAME(sys_ni_syscall)
	.endr
