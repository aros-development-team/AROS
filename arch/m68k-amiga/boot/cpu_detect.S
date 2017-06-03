	.text
	.chip 68060
	.align 4 

/* Detect CPU and FPU type */

	.globl _cpu_detect
_cpu_detect:
	move.l	%a0,%a2
	clr.l	%a2@(4)
	lea		%pc@(cpu_detect_trap_illg),%a1
	move.l	%a1,4*4.w	/* illegal instruction */
	lea		%pc@(fpu_detect_trap_f),%a1
	move.l	%a1,11*4.w	/* f-line */
	bsr		cpu_detect_asm
	move.w	%d0,%d2
	bsr		cpu_detect_fpu_asm
	swap	%d2
	move.w	%d0,%d2
	swap	%d2
	move.l	%d2,%a2@
	rts

cpu_detect_fpu_asm:
	move.l	%sp,%a1
	lea		%sp@(-60),%sp
	move.l	%sp,%a0
	clr.w	(%a0)
	fnop
	fsave	(%a0)
	move.w	#0x8000,%d0
	move.b	1(%a0),%d0
	move.l	%a1,%sp
	rts		/* return to cpu_detect */

fpu_detect_trap_f:
	move.l	%a1,%sp
	moveq	#0,%d0
	rts		/* return to cpu_detect */

cpu_detect_asm:
	move.l	%sp,%a1
	moveq	#0,%d0
	/* VBR is 68010+ */
	movec	%d0,%vbr
	move.w	#0x0001,%d0
 	/* CACR is 68020+ */
	movec	%cacr,%d0
	/* 68020+ or better */
	move.l	#0x00008000,%d0
 	/* enable 68040/060 code cache */
	movec	%d0,%cacr
	movec	%cacr,%d0
 	/* bit 15 still set? */
	tst.w	%d0
 	/* yes, it is 68040, 68060 or 68080 */
	bmi.s	0f
 	/* enable 68020/030 code cache and 68030 data cache */
 	move.l	#0x0101,%d0
	movec	%d0,%cacr
	movec	%cacr,%d0
	/* disable caches */
	moveq	#0,%d1
	movec	%d1,%cacr
 	/* data cache bit still set? */
	btst	#8,%d0
	bne.s	1f /* yes, it is 68030 */
	/* 68020 */
	move.w	#0x0003,%d0
	bra cpu_detect_trap_illg
	/* 68030 */
1:
	move.w	#0x0007,%d0
	bra cpu_detect_trap_illg
	/* 68040, 68060, or 68080 */
0:
	moveq	#0,%d0
	/* disable caches */
	movec	%d0,%cacr
	/* data cache must be invalidated after reset */
	cinva %dc
	/* set transparent translation registers,
	 * allow data caching only in 32-bit fast,
	 * code caching allowed everywhere */
	movec	%d0,%itt1
	move.l	#0x0000e040,%d0
	movec	%d0,%dtt0
	move.l	#0x00ffe000,%d0
	movec	%d0,%dtt1
	movec	%d0,%itt0
	move.w	#0x000f,%d0
 	/* PCR is 68060 or 68080 only */
	movec	%pcr,%d1
	move.l	%d1,%d2
        /* Apollo 68080 has PCR ID 0x0440xxxx */
	and.l	#0xffff0000,%d1
	cmp.l	#0x04400000,%d1
	bcc.s	2f
	move.l	%d2,%d1
	/* 68060 */
	moveq	#0,%d0
	cmp.w	#0x0600,%d1
	bcc.s	3f
	/* rev5 or older 68060 revision
	 * enable I14 and I15 errata workaround
	 */
	bset	#5,%d0
	bra.s	3f
2:	/* Apollo 68080 */
	movec	%pcr,%d0
	move.l	%d0,%a2@(4) // save PCR
	move.w	#0x048f,%d0
	bra.s	cpu_detect_trap_illg
3:	/* Also enables FPU */
	movec	%d0,%pcr
	movec	%pcr,%d0
	move.l	%d0,%a2@(4) // save PCR
	move.w	#0x008f,%d0

cpu_detect_trap_illg:
	move.l %a1,%sp	/* remove exception stack frame */
	rts				/* return to cpu_detect() */
