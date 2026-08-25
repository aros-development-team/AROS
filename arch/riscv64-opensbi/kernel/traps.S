/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: S-mode trap entry for the opensbi-riscv64 target.

    Saves the interrupted state as a struct ExceptionContext
    (see aros/riscv64/cpucontext.h) on the current stack and calls
    krnTrapHandler(ctx, scause, stval). On return the (possibly
    modified) context is restored and execution resumes via sret.

    Frame layout (16-byte aligned, 272 bytes):
      0-224:  x[29]  ra,sp,t0,t1,t2,fp,s1,a0-a7,s2-s11,t3-t6
      232:    pc     (sepc)
      240:    sr     (sstatus - carries the per-task FS dirty state)
      248:    Flags
      256:    fpuContext
      264:    vecContext

    gp and tp are kernel-owned and are deliberately not part of the
    context (matching RISCV_REGSAVE_CNT).
*/

	.text
	.balign	4
	.globl	__trap_entry
	.type	__trap_entry,%function
__trap_entry:
	addi	sp, sp, -272
	sd	ra, 0(sp)
	sd	t0, 16(sp)
	sd	t1, 24(sp)
	sd	t2, 32(sp)
	sd	s0, 40(sp)
	sd	s1, 48(sp)
	sd	a0, 56(sp)
	sd	a1, 64(sp)
	sd	a2, 72(sp)
	sd	a3, 80(sp)
	sd	a4, 88(sp)
	sd	a5, 96(sp)
	sd	a6, 104(sp)
	sd	a7, 112(sp)
	sd	s2, 120(sp)
	sd	s3, 128(sp)
	sd	s4, 136(sp)
	sd	s5, 144(sp)
	sd	s6, 152(sp)
	sd	s7, 160(sp)
	sd	s8, 168(sp)
	sd	s9, 176(sp)
	sd	s10, 184(sp)
	sd	s11, 192(sp)
	sd	t3, 200(sp)
	sd	t4, 208(sp)
	sd	t5, 216(sp)
	sd	t6, 224(sp)
	/* interrupted sp, pc and sstatus */
	addi	t0, sp, 272
	sd	t0, 8(sp)
	csrr	t0, sepc
	sd	t0, 232(sp)
	csrr	t0, sstatus
	sd	t0, 240(sp)
	/* Flags / context pointers */
	sw	zero, 248(sp)
	sd	zero, 256(sp)
	sd	zero, 264(sp)

	mv	a0, sp			/* ctx    */
	csrr	a1, scause
	csrr	a2, stval
	call	krnTrapHandler

	/* restore (the handler may have modified the context).

	   Force SIE off for the whole restore first. This sequence is not
	   re-entrant: an interrupt taken part-way through it (once sstatus, and
	   with it SIE, has been reloaded from the frame) is saved with an sepc
	   pointing inside this code. Resuming that half-restored context re-runs
	   the "ld sp, 8(sp)" below with a stale sp, walking sp up off the top of
	   the task's stack a frame at a time until a load reads adjacent memory
	   as the context. It could also reschedule (the nesting depth is briefly
	   1 again), propagating the bad context into a task's saved frame. Hold
	   SIE clear until the closing sret, which restores the task's real
	   interrupt-enable from the frame's SPIE. */
	csrci	sstatus, 2
	ld	t0, 232(sp)
	csrw	sepc, t0
	/* restore sstatus from the context: carries the task's FS state
	   and its SPIE (pre-trap interrupt-enable) for sret. Mask SIE out of the
	   reloaded value so it stays clear across the register restore below. */
	ld	t0, 240(sp)
	andi	t0, t0, -3
	csrw	sstatus, t0
	ld	ra, 0(sp)
	ld	t2, 32(sp)
	ld	s0, 40(sp)
	ld	s1, 48(sp)
	ld	a0, 56(sp)
	ld	a1, 64(sp)
	ld	a2, 72(sp)
	ld	a3, 80(sp)
	ld	a4, 88(sp)
	ld	a5, 96(sp)
	ld	a6, 104(sp)
	ld	a7, 112(sp)
	ld	s2, 120(sp)
	ld	s3, 128(sp)
	ld	s4, 136(sp)
	ld	s5, 144(sp)
	ld	s6, 152(sp)
	ld	s7, 160(sp)
	ld	s8, 168(sp)
	ld	s9, 176(sp)
	ld	s10, 184(sp)
	ld	s11, 192(sp)
	ld	t3, 200(sp)
	ld	t4, 208(sp)
	ld	t5, 216(sp)
	ld	t6, 224(sp)
	/* Release the in-trap nesting depth here, at the very end of the exit,
	   rather than in krnTrapHandler(): the depth must stay elevated across
	   this whole restore so that a trap taken mid-exit still counts as
	   nested and cannot enter the scheduler and snapshot this half-restored
	   exit state as a task context. t0/t1 are still scratch (restored just
	   below, before sp). */
	la	t0, __riscv64_trap_depth
	lw	t1, 0(t0)
	addiw	t1, t1, -1
	sw	t1, 0(t0)
	ld	t1, 24(sp)
	ld	t0, 16(sp)
	/* Load sp last, directly from the context - the dispatcher may
	   have switched the frame to a different task */
	ld	sp, 8(sp)
	sret
