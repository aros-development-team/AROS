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

#include <hardware/intbits.h>

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

/* Wrapper to work around GCC frame pointer bugs
 */
static inline void Amiga_Paula_IRQ(int irq, UWORD mask, struct ExecBase *SysBase)
{
    /* If we don't have handler, ignore it */
    if (SysBase->IntVects[irq].iv_Code == NULL) {
    	bug("SPURIOUS IRQ: %d mask=0x%04x\n", irq, mask);
    	return;
    }

#if 1
    AROS_UFC5(void, SysBase->IntVects[irq].iv_Code,			\
		    AROS_UFCA(ULONG, mask, D1),				\
		    AROS_UFCA(ULONG, 0xDFF000, A0),			\
		    AROS_UFCA(APTR, SysBase->IntVects[irq].iv_Data, A1),\
		    AROS_UFCA(APTR, SysBase->IntVects[irq].iv_Code, A5),\
		    AROS_UFCA(struct ExecBase *, SysBase, A6));
#else
    asm volatile (
    	    "move.l	%0,%%d1\n"
    	    "move.l	%1,%%a0\n"
    	    "move.l	%2,%%a1\n"
    	    "move.l	%4,%%a6\n"
    	    "move.l	%%a5,%%sp@-\n"
    	    "move.l	%3,%%a5\n"
    	    "jsr.l	(%%a5)\n"
    	    "move.l	%%sp@+,%%a5\n"
    	    :
    	    : "g" (mask),
    	      "g" (0xdff000),
    	      "g" (SysBase->IntVects[irq].iv_Data),
    	      "r" (SysBase->IntVects[irq].iv_Code),
    	      "g" (SysBase)
    	    : "%d0", "%d1", "%a0", "%a1", "%a6");
#endif
}

#define PAULA_IRQ_CHECK(valid_mask) \
    const UWORD irq_mask = valid_mask; \
    UWORD mask = custom_r(INTENAR) & custom_r(INTREQR) & (irq_mask); \
    custom_w(INTREQ, mask);

#define PAULA_IRQ_HANDLE(irq) \
    do { \
    	if ((mask) & (1 << (irq))) \
    	    Amiga_Paula_IRQ(irq, mask, SysBase); \
    } while (0)

#define PAULA_IRQ_EXIT()	\
    /* If the caller was not nested, call core_ExitInterrupt */	\
    if (!(regs->sr & 0x2000))					\
    	core_ExitInterrupt(regs);

static void Amiga_Level_1(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs 0 - Serial port TX done
     *            1 - Disk DMA finished
     *            2 - SoftInt
     */
    PAULA_IRQ_CHECK(INTF_SOFTINT | INTF_DSKBLK | INTF_TBE);

    PAULA_IRQ_HANDLE(INTB_TBE);
    PAULA_IRQ_HANDLE(INTB_DSKBLK);
    PAULA_IRQ_HANDLE(INTB_SOFTINT);

    PAULA_IRQ_EXIT();
}

static void Amiga_Level_2(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs 3 - CIAA/CIAB
     */
    PAULA_IRQ_CHECK(INTF_PORTS);

    PAULA_IRQ_HANDLE(INTB_PORTS);

    PAULA_IRQ_EXIT();
}

static void Amiga_Level_3(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs 4 - Copper
     *            5 - Vert Blank
     *            6 - Blitter
     */
    PAULA_IRQ_CHECK(INTF_COPER | INTF_VERTB | INTF_BLIT);

    PAULA_IRQ_HANDLE(INTB_COPER);
    PAULA_IRQ_HANDLE(INTB_VERTB);
    PAULA_IRQ_HANDLE(INTB_BLIT);

    PAULA_IRQ_EXIT();
}

static void Amiga_Level_4(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs  7 - Audio 0
     *             8 - Audio 1
     *             9 - Audio 2
     *            10 - Audio 3
     */
    PAULA_IRQ_CHECK(INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3);

    PAULA_IRQ_HANDLE(INTB_AUD0);
    PAULA_IRQ_HANDLE(INTB_AUD1);
    PAULA_IRQ_HANDLE(INTB_AUD2);
    PAULA_IRQ_HANDLE(INTB_AUD3);
    
    PAULA_IRQ_EXIT();
}

static void Amiga_Level_5(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs  11 - Serial RX
     *             12 - Disk Sync
     */
    PAULA_IRQ_CHECK(INTF_RBF | INTF_DSKSYNC);

    PAULA_IRQ_HANDLE(INTB_RBF);
    PAULA_IRQ_HANDLE(INTB_DSKSYNC);

    PAULA_IRQ_EXIT();
}

static void Amiga_Level_6(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* Paula IRQs  13 - External 
     *             14 - Copper 'special'
     */
    PAULA_IRQ_CHECK(INTF_EXTER | (1 << 14));

    PAULA_IRQ_HANDLE(INTB_EXTER);

    /* 14 is the Copper 'special' bit. */
    PAULA_IRQ_HANDLE(14);

    PAULA_IRQ_EXIT();
}

static void Amiga_Level_7(regs_t *regs, int id, struct ExecBase *SysBase)
{
    /* NMI - no way around it.
     */
    const UWORD mask = (1 << 15);

    PAULA_IRQ_HANDLE(15);

    /* Don't reschedule on the way out - so don't
     * call PAULA_IRQ_EXIT()
     */
}

const struct M68KException AmigaExceptionTable[] = {
	{ .Id =  11, .Handler = LineF_Decode },
	{ .Id =  25, .Handler = Amiga_Level_1 },
	{ .Id =  26, .Handler = Amiga_Level_2 },
	{ .Id =  27, .Handler = Amiga_Level_3 },
	{ .Id =  28, .Handler = Amiga_Level_4 },
	{ .Id =  29, .Handler = Amiga_Level_5 },
	{ .Id =  30, .Handler = Amiga_Level_6 },
	{ .Id =  31, .Handler = Amiga_Level_7 },
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
