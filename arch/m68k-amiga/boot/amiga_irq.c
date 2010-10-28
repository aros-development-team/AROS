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

#include "m68k_exception.h"
#include "amiga_hwreg.h"
#include "amiga_irq.h"

#include "exec_intern.h"

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
static void Amiga_Paula_IRQ(int id, UWORD SRReg, struct ExecBase *SysBase)
{
	UWORD irqs = reg_r(INTENAR) & reg_r(INTREQR);
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
			reg_w(INTREQ, (1 << i));
		}
	}

	/* If the caller was not superuser, attempt to schedule
	 */
	if (!(SRReg & 0x2000))
		KrnSchedule();

	/* Remove any IRQ masks */
	asm volatile ("move.w #0x2000,%sr");
}

const struct M68KException AmigaExceptionTable[] = {
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
	reg_w(INTENA, 0x7fff);
	/* Clear any requests */
	reg_w(INTREQ, 0x7fff);

	M68KExceptionInit(AmigaExceptionTable, SysBase);
}
