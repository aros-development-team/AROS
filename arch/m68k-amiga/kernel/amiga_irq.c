/*
    copyright © 1995-2010, the aros development team. all rights reserved.
    $id$

    desc: m68k-amiga IRQ handling
    lang: english
 */

#include <aros/kernel.h>
#include <aros/asmcall.h>

#include <exec/resident.h>
#include <exec/execbase.h>
#include <defines/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"

#include "m68k_exception.h"
#include "amiga_irq.h"

#include "exec_intern.h"

/** Interrupts */
#define INTENAR			0x1c
#define INTREQR			0x1e
#define INTENA			0x9a
#define INTREQ			0x9c

/** DMA **/
#define DMACON			0x96

static inline void custom_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}

static inline UWORD custom_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}


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
static void LineF_Decode(regs_t *regs, int id, struct ExecBase *SysBase)
{
	if (*((UWORD *)regs->pc) == KRN_SYSCALL_INST &&
	     regs->a[0] == KRN_SYSCALL_MAGIC &&
	     (regs->sr & 0x2000) == 0) {
	     	/* Move past the instruction */
		regs->pc += 4;	

	     	/* AROS syscall */
		switch (regs->d[0]) {
		case SC_SCHEDULE:
			if (!core_Schedule())
				break;
			/* FALLTHROUGH */
		case SC_SWITCH:
			cpu_Switch(regs);
			/* FALLTHROUGH */
		case SC_DISPATCH:
			cpu_Dispatch(regs);
			break;
		case SC_CAUSE:
			core_ExitInterrupt(regs);
			break;
		}

		return;
	}

	Alert(ACPU_LineF);
	for (;;);
}

static void Amiga_Paula_IRQ(regs_t *regs, int id, struct ExecBase *SysBase)
{
	UWORD irqs = custom_r(INTENAR) & custom_r(INTREQR);
	int i;

	for (i = 0; i < 14; i++) {
		if (irqs & (1 << i)) {
			if (SysBase->IntVects[i].iv_Code != NULL)
				AROS_UFC5(void, SysBase->IntVects[i].iv_Code,
					AROS_UFCA(ULONG, irqs, D1),
					AROS_UFCA(ULONG, 0xDFF000, A0),
					AROS_UFCA(APTR, SysBase->IntVects[i].iv_Data, A1),
					AROS_UFCA(APTR, SysBase->IntVects[i].iv_Code, A5),
					AROS_UFCA(struct ExecBase *, SysBase, A6));
			/* Mark the IRQ as serviced */
			custom_w(INTREQ, (1 << i));
		}
	}

	/* If the caller was not nested, call core_ExitInterrupt
	 */
	if (!(regs->sr & 0x2000))
		core_ExitInterrupt(regs);
}

const struct M68KException AmigaExceptionTable[] = {
	{ .Id =  11, .Handler = LineF_Decode },
	{ .Id =  16, .Handler = NULL },
	{ .Id =  17, .Handler = NULL },
	{ .Id =  18, .Handler = NULL },
	{ .Id =  19, .Handler = NULL },
	{ .Id =  20, .Handler = NULL },
	{ .Id =  21, .Handler = NULL },
	{ .Id =  22, .Handler = NULL },
	{ .Id =  23, .Handler = NULL },
	{ .Id =  24, .Handler = NULL },
	{ .Id =  25, .Handler = Amiga_Paula_IRQ },
	{ .Id =  26, .Handler = Amiga_Paula_IRQ },
	{ .Id =  27, .Handler = Amiga_Paula_IRQ },
	{ .Id =  28, .Handler = Amiga_Paula_IRQ },
	{ .Id =  29, .Handler = Amiga_Paula_IRQ },
	{ .Id =  30, .Handler = Amiga_Paula_IRQ },
	{ .Id =  31, .Handler = Amiga_Paula_IRQ },
	{ .Id =   0, }
};

void AmigaIRQInit(struct ExecBase *SysBase)
{
	/* Disable all interrupts */
	custom_w(INTENA, 0x7fff);
	/* Clear any requests */
	custom_w(INTREQ, 0x7fff);

	M68KExceptionInit(AmigaExceptionTable, SysBase);

	/* Enable DMA */
	custom_w(DMACON, 0x8200);

	/* IRQs will be enabled by the first Enable() in Exec's init */
}
