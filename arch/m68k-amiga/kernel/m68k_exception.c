/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: m68k-amiga bootstrap to exec.
    lang: english
 */

#include <aros/kernel.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/memory.h>

#include "exec_intern.h"

#include "m68k_exception.h"

/* Here's how it's all laid out on the Amiga
 *    M68K Exception
 *      0	Reset: Initial SP
 *      1	Reset: Initial PC (NOTE: Really is SysBase!)
 *      2	Bus Error
 *      3	Address Error
 *      4	Illegal Instruction
 *      5	Divide by Zero
 *      6	CHK Instruction
 *      7	TRAPV Instruction
 *      8	Privileged Instruction
 *      9	Trace
 *      10	Line 1010 Emulator
 *      11	Line 1111 Emulator
 *      12	-
 *      13	-
 *      14	Format Error
 *      15	Uninitilaized Interrupt Vector
 *      16	-
 *      ..
 *      23	-
 *      24	Spurious Interrupt
 *      25	Level 1 Interrupt
 *      		Paula 0: Serial TX
 *      		Paula 1: Disk DMA done
 *      		Paula 2: Software Int
 *      26	Level 2 Interrupt
 *      		Paula 3: CIA
 *      27	Level 3 Interrupt
 *      		Paula 4: Copper
 *      		Paula 5: Vert Blank
 *      		Paula 6: Blitter
 *      28	Level 4 Interrupt
 *      		Paula 7: Audio 0
 *      		Paula 8: Audio 1
 *      		Paula 9: Audio 2
 *      		Paula 10: Audio 3
 *      29	Level 5 Interrupt
 *      		Paula 11: Serial RX
 *      		Paula 12: Disk Sync
 *      30	Level 6 Interrupt
 *      		Paula 13: External
 *      		Paula 14: Copper (special)
 *      31	Level 7 Interrupt
 *      		Paula 15: NMI
 *      32	TRAP #0
 *      ..
 *      47	TRAP #15
 *      48	-
 *      ..
 *      63	-
 *      64	User 1
 *      ..
 *      255	User 191
 */

/* The stack frame here:
 *     Return PC          +(0 + 2 + 4)
 *     Return SP          +(0 + 2)
 *     Exception struct * +(0)
 * SP ->
 *
 * When we call M68KExceptionAction:
 *     D0-D1/A0-A1/A6		(5 * 4)	<= NO TOUCHING!
 *     SysBase			(4)	A6
 *     SP			(4)	D1
 *     Exception struct *	(4)	D0
 *
 * When we come back:
 *     Drop SysBase, SP, and Exception args
 *     Restore D0-D1/A0-A1/A6
 *     Drop the Exception # word
 *     RTE
 */
extern void M68KExceptionHelper(void);
asm (
	"	.text\n"
	"	.globl M68KExceptionHelper\n"
	"M68KExceptionHelper:\n"
	"	movem.l	%d0-%d7/%a0-%a6,%sp@-\n"// Push everything - SP is now 'regs_t'
	"	move.l	%sp@(15*4),%d1\n"	// Exception *
	"	move.l	%sp,%a0\n"		// Supervisor SP
	"	btst	#5,%sp@(4*16)\n"	// From Supervisor?
	"	bne.s	0f\n"			// Nope.
	"	move.l	%usp,%a0\n"
	"0:\n"
	"	move.l	%a0,%sp@(4*15)\n"	// Fix up SP in regs
	"	move.l	%sp,%d0\n"		// regs_t
	"	move.l	4, %a6\n"		// Global SysBase
	"	move.l	%a6, %sp@-\n"		// Push SysBase
	"	move.l	%d1, %sp@-\n"		// Push Exception *
	"	move.l	%d0, %sp@-\n"		// Push regs_t *
	"	jsr	M68KExceptionAction\n"
	"	lea	%sp@(12),%sp\n"		// Drop all stack args
	"	movem.l	%sp@+,%d0-%d7/%a0-%a5\n"	// Restore all but a6
	"	btst	#5,%sp@(4)\n"		// Back to Supervisor?
	"	bne.s	1f\n"			// No:
	"	move.l	%sp@(4),%a6\n"		//   Get USP from regs_t
	"	move.l	%a6,%usp\n"		//   Set USP (FALLTHROUGH)
	"1:\n"					// Yes:
	"	move.l	%sp@+,%a6\n"		//   Restore A6
	"	addq.l	#4,%sp\n"		//   Pop off A7
	"	rte\n"				//   And return
);

#undef kprintf

#define PARANOIA_STACK
void M68KExceptionAction(regs_t *regs, struct M68KException *Exception, struct ExecBase *SysBase)
{
#ifdef PARANOIA_STACK
	extern void breakpoint(void);
	if (regs->sr & 0x2000) {
		if ((APTR)regs->a[7] < (SysBase->SysStkLower+0x100) || (APTR)(regs->a[7]-1) > SysBase->SysStkUpper) {
			bug("Supervisor: YOUR STACK A SPLODE!\n");
			breakpoint();
		}
	} else {
		struct Task *t = SysBase->ThisTask;
		extern void *_us_stack_upper, *_us_stack_lower;
		if ((IPTR)t->tc_SPUpper == ~0 && (IPTR)t->tc_SPLower == 0) {
			t->tc_SPUpper = (void *)(&_us_stack_upper-1);
			t->tc_SPLower = (void *)&_us_stack_lower;
		}
		if ((APTR)regs->a[7] < (t->tc_SPLower+0x100) || (APTR)(regs->a[7]-1) > t->tc_SPUpper) {
			bug("[%s]: YOUR STACK A SPLODE!\n", t->tc_Node.ln_Name);
			breakpoint();
		}
	}
#endif

	if (Exception->Handler == NULL) {
		kprintf("-- Exception %d\n", Exception->Id);
		Alert(AN_BogusExcpt);
		for (;;);
	}

	Exception->Handler(regs, Exception->Id, SysBase);
}

/* We assume that the caller has already set up
 * the exceptions to a 'reasonable' default. These
 * are only the overrides for AROS.
 */
void M68KExceptionInit(const struct M68KException *Table, struct ExecBase *SysBase)
{
	IPTR *exception = (IPTR *)0;	/* Exception base is at 0 */
	UWORD *jmptab;
	int i;
	int size;

	for (size = 0; Table[size].Id > 0; size++);

	/* A little explanation. jmptab will be 
	 * constructed as follows:
	 *    move.l &Table[i], %sp@+
	 *      0x2f3c (&Table[i] >> 16) (&Table[i] & 0xffff)
	 *    jmp %pc@(((size - 1) - i) * (5 * sizeof(UWORD)) + 2)
	 *      0x4efa (((size - 1) - i) * (5 * sizeof(UWORD)) + 2)
	 *    ...
	 *    ...
	 *    jmp M68KExceptionHelper
	 *      0x4ef9 (M68KExceptionHelper >> 16) (M68KExceptionHelper & 0xffff)
	 *
	 * NOTICE: jmptab will never be freed! */
	jmptab = AllocMem(size * (5 * sizeof(UWORD)) + 3 * sizeof(UWORD), 0);

	for (i = 0; i < size; i++, jmptab += 5) {
		jmptab[0] = 0x2f3c;	// movel #...,%sp@-
		jmptab[1] = ((IPTR)(&Table[i]) >> 16) & 0xffff;
		jmptab[2] = ((IPTR)(&Table[i]) >>  0) & 0xffff;
		jmptab[3] = 0x4efa;	// jmp %pc@...
		jmptab[4] = ((size - 1) - i) * (5 * sizeof(UWORD)) + 2;
		exception[Table[i].Id] = (IPTR)(&jmptab[0]);
	}
	jmptab[0] = 0x4ef9;		// jmp ....
	jmptab[1] = ((IPTR)(M68KExceptionHelper) >> 16) & 0xffff;
	jmptab[2] = ((IPTR)(M68KExceptionHelper) >>  0) & 0xffff;

	/* We're all set up now! */
}
